#ifndef _SYNC_H
#define _SYNC_H

#include <limits.h>
#include "prjdef.h"
#include "list.h"
#include "queue.h"
#include "task.h"

#define ERR_TIME_OUT  	-1

/************************** counting semaphore ****************************/

typedef struct counting_semaphore {
	volatile int count;
	int max;
	list block_list;
} cntsem;
typedef cntsem* cntsem_handle;

void sem_init(cntsem_handle s, int max_cnt, int init_cnt);
cntsem_handle sem_create(int max_cnt, int init_cnt);
int sem_delete(cntsem_handle s);

int sem_wait(cntsem_handle s, u_int wait_ticks);
int sem_signal(cntsem_handle s);
int sem_signal_isr(cntsem_handle s);


/******************************** mutex **********************************/

typedef struct mutual_exclusion{
	tcb_t *lock_owner;
	list block_list;
	u_int bkp_prio;
} mutex;
typedef mutex* mutex_handle;

void mutex_init(mutex_handle mtx);
mutex_handle mutex_create(void);
int mutex_delete(mutex_handle mtx);

void mutex_lock(mutex_handle mtx);
void mutex_unlock(mutex_handle mtx);


/**************************** message queue ******************************/

typedef struct message_queue {
	queue  que;
	list   wb_list;				// write block list
	list   rb_list;				// read block list
} msgque;
typedef msgque* msgque_handle;


#define MSGQUE_LEN(pmsgq) ((pmsgq)->que.len)
#define MSGQUE_FULL(pmsgq) ((pmsgq)->que.len == (pmsgq)->que.max)

void msgque_init(msgque_handle mq, int nitems, int size);
msgque_handle msgque_create(int nitems, int size);
int msgque_delete(msgque_handle mq);

int msgque_push(msgque_handle mq, void *item, u_int wait_ticks);
int msgque_try_push(msgque *mq, u_int wait_ticks);
void msgque_overwrite(msgque_handle mq, void *item);
void msgque_overwrite_isr(msgque *mq, void *item);

int msgque_front(msgque_handle mq, void *buf, u_int wait_ticks);
void msgque_pop(msgque_handle mq);


/******************************** event **********************************/

typedef u_short evt_bits_t;
typedef struct event_group {
	evt_bits_t evt_bits;
	list block_list;
} evt_group;
typedef evt_group* evt_group_handle;

#define EVT_GROUP_OPT_OR 	 0
#define EVT_GROUP_OPT_AND	 1

void evt_group_init(evt_group_handle grp, evt_bits_t init_bits);
evt_group_handle evt_group_create(evt_bits_t init_bits);
int evt_group_delete(evt_group_handle grp);

int evt_group_wait(evt_group_handle grp, evt_bits_t bits, bool clr, int opt, u_int wait_ticks);
void evt_group_set(evt_group_handle grp, evt_bits_t bits);
void evt_group_set_isr(evt_group_handle grp, evt_bits_t bits);
void evt_group_clear(evt_group_handle grp, evt_bits_t bits);
void evt_group_clear_isr(evt_group_handle grp, evt_bits_t bits);

#endif
