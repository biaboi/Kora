#ifndef _SYNC_H
#define _SYNC_H

#include <limits.h>
#include "KoraDef.h"
#include "klist.h"
#include "queue.h"
#include "task.h"

#define ERR_TIME_OUT  	-1


/************************** counting semaphore ****************************/

typedef struct counting_semaphore {
	volatile int    count;
	int             max;
	kernel_list     block_list;
} cntsem;

typedef cntsem* sem_t;

void sem_init(sem_t s, int max_cnt, int init_cnt);
sem_t sem_create(int max_cnt, int init_cnt);
int sem_delete(sem_t s);

int sem_wait(sem_t s, u_int wait_ticks);
int sem_signal(sem_t s);
int sem_signal_isr(sem_t s);


/******************************** mutex **********************************/

typedef struct mutual_exclusion{
	tcb_t          *lock_owner;
	kernel_list     block_list;
	u_int           bkp_prio;
} mutex;

typedef mutex* mutex_t;

void mutex_init(mutex_t mtx);
mutex_t mutex_create(void);
int mutex_delete(mutex_t mtx);

void mutex_lock(mutex_t mtx);
void mutex_unlock(mutex_t mtx);


/**************************** message queue ******************************/

typedef struct message_queue {
	queue  que;
	kernel_list   wb_list;				// write block list
	kernel_list   rb_list;				// read block list
} msgque;

typedef msgque* msgq_t;


#define MSGQUE_LEN(pmsgq) ((pmsgq)->que.len)
#define MSGQUE_FULL(pmsgq) ((pmsgq)->que.len == (pmsgq)->que.max)

void msgq_init(msgq_t mq, int nitems, int size);
msgq_t msgq_create(int nitems, int size);
int msgq_delete(msgq_t mq);

int msgq_push(msgq_t mq, void *item, u_int wait_ticks);
int msgq_try_push(msgq_t mq, u_int wait_ticks);
void msgq_overwrite(msgq_t mq, void *item);
void msgq_overwrite_isr(msgq_t mq, void *item);

int msgq_front(msgq_t mq, void *buf, u_int wait_ticks);
void msgq_pop(msgq_t mq);


/******************************** event **********************************/

typedef u_int evt_bits_t;

typedef struct event_group {
	evt_bits_t     evt_bits;
	kernel_list    block_list;
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


#endif // _SYNC_H
