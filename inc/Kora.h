#ifndef _KORA_H
#define _KORA_H

#include "KoraDef.h"
#include "alloc.h"
#include "assert.h"
#include "list.h"
#include "queue.h"
#include "byteBuffer.h"
#include <limits.h>

#define KORA_VERSION    "0.83"

/********************************** tasks ************************************/

/*
                higher addrress
    +---------------------------------+
    |                                 |	
    |       task control block        |
    |                                 |
    +---------------------------------|  <- top_of_stack (when stack empty)
    |                                 |
    |          run time stack         |
    |                                 |
    +---------------------------------+  <- start_addr

*/

typedef void (*transfer_t)(char *, int);

typedef enum {
    running, ready, sleeping, blocking, suspend}
task_stat;


typedef struct __tcb {
    u_char         *top_of_stack;
    u_int           magic;            // used for check if tcb is accidentally overwritten 
    u_int           occupied_tick;    // used for roughly calculate the CPU usage
    u_char         *start_addr;
    u_int           priority;
    char            name[CFG_TASK_NAME_LEN];
    u_int           min_stack;        // store the minimum remaining stack size
    task_stat       state;
    u_int           evt_flags;        // used in event group
    list_node_t     state_node;       // state_node will be only mounted on ready_list or sleep_list
    list_node_t     event_node;       
    list_node_t     link_node;        // once the task is created, it is mounted to the all_tasks list 
} tcb_t;


typedef tcb_t* task_handle;


#define PRIORITY_LOWEST     (u_int)(CFG_MAX_PRIOS-1)
#define PRIORITY_HIGHEST    (u_int)1
#define FOREVER      UINT_MAX

// Used as a parameter of foreach_task() to process tasks.
typedef void (*task_process_t)(task_handle, void *);

task_handle task_init(vfunc code, const char *name, void *para, u_int prio, u_char *stk, int size);
task_handle task_create(vfunc code, const char *name, void *para, u_int prio, int size);
task_handle qcreate(vfunc code, int priority, int size);
void task_delete(task_handle tsk);
void task_delete_isr(task_handle tsk);

int task_modify_priority(task_handle tsk, int new);
void task_ready(task_handle tsk);
void task_ready_isr(task_handle tsk);
void sleep(u_int xtick);
void block(list_t *blklst, u_int wait_ticks);
void block_isr(task_handle tsk, list_t *blklst, u_int wait_ticks);
void task_suspend(task_handle tsk);
void task_suspend_isr(task_handle tsk);

task_stat task_state(task_handle tsk);
u_int task_left_sleep_tick(task_handle tsk);
char* task_name(task_handle tsk);
task_handle task_find(char *name);
void foreach_task(task_process_t process, void *para);

void enter_critical(void);
void exit_critical(void);
void disable_task_switch(void);
void enable_task_switch(void);
bool is_scheduler_running(void);
task_handle self(void);
task_handle os_get_running_task(void);
int os_get_tick(void);
int os_get_cpu_utilization(void);
int os_get_task_num(void);

void Kora_start(void);



typedef enum {ipc_sem, ipc_mtx, ipc_msgq, ipc_evt, ipc_sq} ipc_type;


/************************** counting semaphore ****************************/

typedef struct counting_semaphore {
	volatile int   count;
	int            size;
	list_t         block_list;
} cntsem;

typedef cntsem* sem_t;

void sem_init(sem_t s, int max_cnt, int init_cnt);
sem_t sem_create(int max_cnt, int init_cnt);
int sem_delete(sem_t s);

int sem_wait(sem_t s, u_int wait_ticks);
int sem_peek(cntsem *s, u_int wait_ticks);
int sem_signal(sem_t s);
int sem_signal_isr(sem_t s);


/******************************** mutex **********************************/

typedef struct mutual_exclusion{
	tcb_t       *lock_owner;
	list_t       block_list;
	u_int        bkp_prio;
} mutex;

typedef mutex* mutex_t;

void mutex_init(mutex_t mtx);
mutex_t mutex_create(void);
int mutex_delete(mutex_t mtx);

void mutex_lock(mutex_t mtx);
void mutex_unlock(mutex_t mtx);


/**************************** message queue ******************************/

typedef struct message_queue {
	queue      que;
	list_t     wb_list;				// write block list
	list_t     rb_list;				// read block list
} msgque;

typedef msgque* msgq_t;


#define MSGQUE_LEN(pmsgq) ((pmsgq)->que.len)
#define MSGQUE_FULL(pmsgq) ((pmsgq)->que.len == (pmsgq)->que.max)

void msgq_init(msgq_t mq, int nitems, int size);
msgq_t msgq_create(int nitems, int size);
int msgq_delete(msgq_t mq);

int msgq_push(msgq_t mq, void *item, u_int wait_ticks);
int msgq_waitfor_push(msgq_t mq, u_int wait_ticks);
void msgq_overwrite(msgq_t mq, void *item);
void msgq_overwrite_isr(msgq_t mq, void *item);

int msgq_front(msgq_t mq, void *buf, u_int wait_ticks);
void msgq_pop(msgq_t mq);


/******************************** event **********************************/

typedef u_int evt_bits_t;

typedef struct event_group {
	evt_bits_t     evt_bits;
	list_t         block_list;
} evt_group;

typedef evt_group* event_t;

#define EVT_GROUP_OPT_OR 	 0
#define EVT_GROUP_OPT_AND	 1

void evt_group_init(event_t grp, evt_bits_t init_bits);
event_t evt_group_create(evt_bits_t init_bits);
int evt_group_delete(event_t grp);

int evt_wait(event_t grp, evt_bits_t bits, bool clr, int opt, u_int wait_ticks);
void evt_set(event_t grp, evt_bits_t bits);
void evt_set_isr(event_t grp, evt_bits_t bits);
void evt_clear(event_t grp, evt_bits_t bits);
void evt_clear_isr(event_t grp, evt_bits_t bits);


/******************************** byte buffer **********************************/

typedef struct {
    byte_buffer   bbf;
    list_t        rb_list;  // read_block_list
    list_t        wb_list;  // write_block_list
} streamq;

typedef streamq* streamq_t;


void streamq_init(streamq_t sq, void *buf, int buf_size);
streamq_t streamq_create(int buf_size);
int streamq_delete(streamq_t sq);
int streamq_push(streamq_t sq, void *data, u_short size, u_int wait_ticks);
int streamq_push_isr(streamq_t sq, void *data, u_short size);
int streamq_front(streamq_t sq, void *output, u_int wait_ticks);
int streamq_front_pointer(streamq_t sq, void **pointer, u_short *outlen, u_int wait_ticks);
void streamq_pop(streamq_t sq);

/******************************** kernel hooks **********************************/

typedef enum {
    hook_task_switched_isr = 0,
    hook_task_delete,
    hook_idle,
    hook_stack_overf_isr,
    hook_systick_isr,
    hook_tick_reset,

    kernel_hook_nums
} kernel_hooks_t;


#if CFG_USE_KERNEL_HOOKS
    extern   vfunc    kernel_hooks[kernel_hook_nums];
    #define  KERNEL_HOOK_ADD(x, func)  (kernel_hooks[x] = (func))
    #define  KERNEL_HOOK_DEL(x)        (kernel_hooks[x] = NULL)
#else
    #define  KERNEL_HOOK_ADD(x, func)  ((void)0)
    #define  KERNEL_HOOK_DEL(x)        ((void)0)
#endif


#endif  // _KORA_H
