#ifndef _TASK_H
#define _TASK_H

#include "KoraConfig.h"
#include "klist.h"
#include <stddef.h>

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

typedef 
    enum task_stat {running, ready, sleeping, blocking, suspend}
task_stat;


typedef struct __tcb {
    u_char         *top_of_stack;
    u_int           magic_n;               // used for check if tcb is accidentally overwritten 
    u_int           occupied_tick;
    u_char         *start_addr;
    u_int           priority;
    char            name[CFG_TASK_NAME_LEN];
    u_int           min_stack;
    task_stat       status;
    kernel_node     state_node;         // state_node will be only mounted on ready_list or sleep_list
    kernel_node     event_node;          
    struct __tcb   *tcb_next;
} tcb_t;


typedef tcb_t* task_handle;


#define PRIORITY_LOWEST     (u_int)(CFG_MAX_PRIOS-1)
#define PRIORITY_HIGHEST    (u_int)1


#define STATE_NODE_TO_TCB(pnode) (tcb_t*)((u_int)(pnode) - offsetof(tcb_t, state_node))
#define EVENT_NODE_TO_TCB(pnode) (tcb_t*)((u_int)(pnode) - offsetof(tcb_t, event_node))

#define FOREVER UINT_MAX

task_handle task_init(vfunc code, const char *name, void *para, u_int prio, u_char *stk, int size);
task_handle task_create(vfunc code, const char *name, void *para, u_int prio, int size);
task_handle qcreate(vfunc code, int priority, int size);
void task_delete(task_handle tsk);

// warning: function task_ready() and block() will lead to thread switch immediately,
//          to protect task from interruption, must enter these function with enter_cretital()

int modify_priority(task_handle tsk, int new);
void task_ready(task_handle tsk);
void task_ready_isr(task_handle tsk);
void sleep(u_int xtick);
void block(kernel_list *blklst, u_int wait_ticks);
void block_isr(task_handle tsk, kernel_list *blklst, u_int wait_ticks);
void task_suspend(task_handle tsk);
void task_suspend_isr(task_handle tsk);

task_stat get_task_stat(task_handle tsk);
u_int get_task_left_sleep_tick(task_handle tsk);
char* get_task_name(task_handle tsk);
task_handle traversing_tasks(int opt);
task_handle find_task(char *name);

/****************************** scheduler state ******************************/

void disable_task_switch(void);
void enable_task_switch(void);
bool is_scheduler_running(void);
task_handle get_running_task(void);
int get_os_tick(void);
int get_cpu_util(void);

#endif
