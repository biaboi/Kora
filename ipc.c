/*
 * Kora rtos
 * Copyright (c) 2024 biaboi
 *
 * This file is part of this project and is licensed under the MIT License.
 * See the LICENSE file in the project root for full license information.
 */

#include "KoraConfig.h"
#include "Kora.h"
#include <string.h>


/**
 * @file ipc.c
 * @brief Definitions of inter-process communication (IPC) structures.
 *
 * This file provides the definitions and initializations of various IPC structures,
 * including semaphores, mutexes, message queues, event groups, and stream queues.
 * These components are essential for managing synchronization and communication
 * between tasks or processes within the system.
 *
 * Structures included:
 * - Semaphore
 * - Mutex
 * - Message Queue
 * - Event Group
 * - Stream Queue
 */

#define EVENT_NODE_TO_TCB(pnode) ((tcb_t*)( (u_int)(pnode) - offsetof(tcb_t, event_node)) )

static void wakeup(list_t *blklst){
	if (LIST_NOT_EMPTY(blklst)){
		list_node_t *first = blklst->dmy.next;
		task_ready(EVENT_NODE_TO_TCB(first));
		enter_critical();
	}
}


static void wakeup_isr(list_t *blklst){
	if (LIST_NOT_EMPTY(blklst)){
		list_node_t *first = blklst->dmy.next;
		task_ready_isr(EVENT_NODE_TO_TCB(first));	
	}
}


extern tcb_t * volatile current_tcb;
extern u_int lock_nesting;

/************************** counting semaphore ****************************/
/*
	Note: in most cases, "signal" operation is not that important, 
	so it will not be block if exceed the maximum counting.
*/

void sem_init(cntsem *s, int max_cnt, int init_cnt){
	os_assert(max_cnt >= init_cnt);

	s->size = max_cnt;
	s->count = init_cnt;
	list_init(&s->block_list);
}


cntsem* sem_create(int max_cnt, int init_cnt){
	cntsem *s = malloc(sizeof(cntsem));
	if (s == NULL)
		return NULL;

	sem_init(s, max_cnt, init_cnt);
	return s;
}


int sem_delete(cntsem *s){
	if (!is_heap_addr(s))
		return RET_FAILED;

	if (LIST_IS_EMPTY(&s->block_list)){
		queue_free(s);
		return RET_SUCCESS;
	}
	return RET_FAILED;
}


/*
@ brief: Wait for the semaphore to be ready and take one.
@ retv: RET_SUCCESS / RET_FAILED(timeout)
@ note: If wait_ticks == 0, Whether the operation is successful or not,
        it will exit immediately without triggering an schedule
*/
int sem_wait(cntsem *s, u_int wait_ticks){
	enter_critical();

	while (s->count <= 0){
		if (wait_ticks == 0){
			exit_critical();
			return RET_FAILED;
		}
		block(&s->block_list, wait_ticks);
		if (s->count > 0)
			break;

		wait_ticks = task_left_sleep_tick(current_tcb);
	}

	s->count -= 1;
	exit_critical();
	return s->count;
}


/*
@ brief: Wait for the semaphore to be ready but do not take.
@ retv: RET_SUCCESS / RET_FAILED.
*/
int sem_peek(cntsem *s, u_int wait_ticks){
	enter_critical();

	while (s->count <= 0){
		if (wait_ticks == 0){
			exit_critical();
			return RET_FAILED;
		}

		block(&s->block_list, wait_ticks);
		if (s->count > 0)
			break;

		wait_ticks = task_left_sleep_tick(current_tcb);
	}

	exit_critical();
	return RET_SUCCESS;
}


/*
@ brief: Release a semaphore. 
@ retv: RET_FAILED -> timeout
        Other -> semaphore current count
*/
int sem_signal(cntsem *s){
	os_assert(lock_nesting == 0);

	enter_critical();
	if (s->count >= s->size){
		exit_critical();
		return RET_FAILED;
	}
	s->count += 1;
	wakeup(&s->block_list);
	exit_critical();
	return s->count;
}


int sem_signal_isr(cntsem *s){
	if (s->count >= s->size){
		return RET_FAILED;
	}
	s->count += 1;
	wakeup_isr(&s->block_list);
	return s->count;
}


/******************************** mutex ********************************/
/*
	Note: mutex is the only ipc that can handles priority inversion correctly.
*/

void mutex_init(mutex* mtx){
	mtx->bkp_prio = 0;
	mtx->lock_owner = NULL;
	list_init(&mtx->block_list);
}


mutex* mutex_create(void){
	mutex *mtx = malloc(sizeof(mutex));
	if (mtx == NULL)
		return NULL;

	mutex_init(mtx);
	return mtx;
}


int mutex_delete(mutex *mtx){
	if (!is_heap_addr(mtx))
		return RET_FAILED;

	if (mtx->lock_owner == NULL){
		queue_free(mtx);
		return RET_SUCCESS;
	}
	return RET_FAILED;
}


/*
@ brief: Get a mutex.
*/
void mutex_lock(mutex *mtx){
	os_assert(lock_nesting == 0);

	enter_critical();
	while (1){
		if (mtx->lock_owner == NULL){
			mtx->lock_owner = current_tcb;
			exit_critical();
			return;
		}

		// handle priority inversion
		tcb_t *owner = mtx->lock_owner;
		int owner_prio = owner->priority;
		int cur_prio = current_tcb->priority;

		// priority promote
		if (owner_prio > cur_prio)
			mtx->bkp_prio = task_modify_priority(owner, cur_prio);

		block(&mtx->block_list, FOREVER);
	}
}


/*
@ brief: Release a mutex.
*/
void mutex_unlock(mutex *mtx){
	os_assert(lock_nesting == 0);

	enter_critical();
	tcb_t *owner = mtx->lock_owner;
	mtx->lock_owner = NULL;

	// recover owner_task's original priority
	if (mtx->bkp_prio != 0){
		task_modify_priority(owner, mtx->bkp_prio);
		mtx->bkp_prio = 0;
	}

	wakeup(&mtx->block_list);
	exit_critical();
	return;
}

/******************************** message queue ********************************/

void msgq_init(msgque *mq, int nitems, int size){
	void *buf = (void*)((u_int)mq + sizeof(msgque));
	queue_init((queue*)mq, buf, nitems, size);

	list_init(&mq->wb_list);
	list_init(&mq->rb_list);
}


msgque* msgq_create(int nitems, int size){
	if (nitems == 0 || size == 0)
		return NULL;
	msgque *mq = malloc(sizeof(msgque) + nitems*size);
	if (!mq)
		return NULL;

	msgq_init(mq, nitems, size);
	return mq;
}


int msgq_delete(msgque *mq){
	int stat = ((queue*)mq)->len | mq->wb_list.list_len 
					   		     | mq->rb_list.list_len;
	if (stat != 0)
		return RET_FAILED;
	if (is_heap_addr(mq))
		queue_free(mq);

	return RET_SUCCESS;
}


/*
@ brief: Push an item into message queue.
@ retv: RET_SUCCESS / RET_FAILED
*/
int msgq_push(msgque *mq, void *item, u_int wait_ticks){
	os_assert(lock_nesting == 0);

	enter_critical();
	while (1){
		if (!MSGQUE_FULL(mq)){
			queue_push((queue*)mq, item);
			wakeup(&mq->rb_list);
			exit_critical();
			return RET_SUCCESS;
		}

		if (wait_ticks == 0){
			exit_critical();
			return RET_FAILED;
		}

		block(&mq->wb_list, wait_ticks);
		wait_ticks = task_left_sleep_tick(current_tcb);
	}
}


/*
@ brief: Waiting until the queue has space.
@ retv: RET_SUCCESS / RET_FAILED
*/
int msgq_waitfor_push(msgque *mq, u_int wait_ticks){
	os_assert(lock_nesting == 0);

	enter_critical();
	while (1){
		if (!MSGQUE_FULL(mq)){
			exit_critical();
			return RET_SUCCESS;
		}

		if (wait_ticks == 0){
			exit_critical();
			return RET_FAILED;
		}

		block(&mq->wb_list, wait_ticks);	
		wait_ticks = task_left_sleep_tick(current_tcb);
	}
}


/*
@ brief: Forced write, if the queue is full, the item at the 
         beginning of the queue will be overwritten.
*/
void msgq_overwrite(msgque *mq, void *item){
	enter_critical();

	queue_push((queue*)mq, item);
	wakeup(&mq->rb_list);

	exit_critical();
}


void msgq_overwrite_isr(msgque *mq, void *item){
	queue_push((queue*)mq, item);
	wakeup_isr(&mq->rb_list);
}


/*
@ brief: Get the front item in queue.
@ note: Only read item, must use msgque_pop() to pop item.
*/ 
int msgq_front(msgque *mq, void *buf, u_int wait_ticks){
	enter_critical();
	while (1){
		if (!QUEUE_IS_EMPTY((queue*)mq)){
			queue_front((queue*)mq, buf);
			exit_critical();
			return RET_SUCCESS;
		}

		if (wait_ticks == 0){
			exit_critical();
			return RET_FAILED;
		}

		block(&mq->rb_list, wait_ticks);
		wait_ticks = task_left_sleep_tick(current_tcb);
	}
}


/*
@ brief: Pop the first item of the queue.
*/
void msgq_pop(msgque *mq){
	enter_critical();
	queue *que = &mq->que;
	if (que->len > 0){
		que->front = (que->front+1) % que->max;
		que->len -= 1;
		wakeup(&mq->wb_list);
	}
	exit_critical();
}



/******************************** event **********************************/
/*
	Use the variable 'evt_flags' int tcb to store bits.
	Up to support 24 bits event.
	Use the 30th bit of evt_flags to store the condition (or/and).
*/

void evt_group_init(event_t grp, evt_bits_t init_bits){
	grp->evt_bits = init_bits;
	list_init(&grp->block_list);
}


event_t evt_group_create(evt_bits_t init_bits){
	event_t grp = malloc(sizeof(cntsem));
	if (grp == NULL)
		return NULL;

	evt_group_init(grp, init_bits);
	return grp;
}


int evt_group_delete(event_t grp){
	if (!is_heap_addr(grp))
		return RET_FAILED;

	if (LIST_IS_EMPTY(&grp->block_list)){
		queue_free(grp);
		return RET_SUCCESS;
	}
	return RET_FAILED;
}


/*
@ brief: Check whether the current bits meet the requirements.
*/
static bool is_bits_satisfy(event_t grp, u_int evt_val){
	char opt = evt_val >> 30;
	evt_bits_t seted_bits = grp->evt_bits;
	evt_bits_t req_bits = evt_val & 0x00FFFFFF;

	if (opt == EVT_GROUP_OPT_OR){
		seted_bits &= (~req_bits);
		if (seted_bits == grp->evt_bits)
			return false;
	}
	else {
		seted_bits |= req_bits;
		if (seted_bits != grp->evt_bits)
			return false;
	}
	return true;
}


/*
@ brief: Wait events happend.
@ param: clr-> if the conditions are met, whether to clear the corresponding flag bit after the task is awakened
         opt-> the condition under which the event occurs is bit and/or
*/
int evt_wait(event_t grp, evt_bits_t bits, bool clr, int opt, u_int wait_ticks){
	os_assert(bits < 0x01000000);

	enter_critical();
	u_int *flags = &(current_tcb->evt_flags);
	*flags = (u_int)bits;
	*flags |= (opt << 30);

	if (is_bits_satisfy(grp, *flags) == false){
		block(&grp->block_list, wait_ticks);

		wait_ticks = task_left_sleep_tick(current_tcb);
		if (wait_ticks == 0){
			exit_critical();
			return RET_FAILED;
		}
	}

	if (clr == true)
		grp->evt_bits &= (~bits);

	exit_critical();
	return RET_SUCCESS;
}


/*
@ brief: Set event group bits.
*/
void evt_set(event_t grp, evt_bits_t bits){
	enter_critical();

	grp->evt_bits |= bits;
	list_node_t *iter = &grp->block_list.dmy;

	while ((iter = iter->next) != &grp->block_list.dmy){
		task_handle tsk = EVENT_NODE_TO_TCB(iter);
		if (is_bits_satisfy(grp, tsk->evt_flags)){
			task_ready(tsk);
			enter_critical();
		}
	}

	exit_critical();	
}


void evt_set_isr(event_t grp, evt_bits_t bits){
	grp->evt_bits |= bits;
	list_node_t *iter = &grp->block_list.dmy;

	while ((iter = iter->next) != &grp->block_list.dmy){
		task_handle tsk = EVENT_NODE_TO_TCB(iter);
		if (is_bits_satisfy(grp, tsk->evt_flags))
			task_ready_isr(tsk);
	}
}


/*
@ brief: Clear all bits.
*/
void evt_clear(event_t grp, evt_bits_t bits){
	enter_critical();

	grp->evt_bits &= (~bits);

	exit_critical();
}


void evt_clear_isr(event_t grp, evt_bits_t bits){
	grp->evt_bits &= (~bits);
}


/******************************* stream queue *********************************/
/*
	Stream queue is an ipc structure for handling continuous non-fixed-length byte data, 
	similar to the stream buffer in Freertos but support multiple writes and read.

	Data is stored and retrieved in segments by using structure byte_buffer.
*/

/*
@ param: Buf -> body of the buffer, excluding streamq header.
         Buf_size -> size of buffer.
@ warning: The buf_size parameter must include the size of the streamq structure.
          The actual size available for the data buffer is 'buf_size - sizeof(streamq)'.
*/
void streamq_init(streamq_t sq, void *buf, int buf_size){
	byte_buffer_init(&sq->bbf, buf, buf_size);
	list_init(&sq->wb_list);
	list_init(&sq->rb_list);
}


/*
@ brief: Dynamically creates a stream queue with the given buffer size.
@ param: buf_size -> the size of the stream queue data buffer (excluding header).
@ note: Tha actually size of memory alloced is buf_size + sizeof(streamq).
*/
streamq_t streamq_create(int buf_size){
	if (buf_size == 0)
		return NULL;

	streamq_t sq = malloc(sizeof(streamq) + buf_size);
	if (sq == NULL){
		return NULL;
	}
	streamq_init(sq, (char*)sq + sizeof(streamq), buf_size);
	return sq;
}


int streamq_delete(streamq_t sq){
	if (LIST_NOT_EMPTY(&sq->wb_list) || LIST_NOT_EMPTY(&sq->rb_list))
		return RET_FAILED;

	if (!is_heap_addr(sq))
		return RET_FAILED;

	queue_free(sq);
	return RET_SUCCESS;
}


/*
@ brief: Push the data into the byte buffer.
         If the space of the byte buffer is insufficient, 
         push the data each time until the byte buffer is full.
@ retv:  RET_SUCCESS / RET_FAILED.
*/
int streamq_push(streamq_t sq, void *data, u_short size, u_int wait_ticks){
	os_assert(lock_nesting == 0);

	enter_critical();
	while (1){
		int ret = byte_buffer_push(&sq->bbf, data, size);
		if (ret != RET_FAILED){
			wakeup(&sq->rb_list);
			exit_critical();
			return RET_SUCCESS;
		}

		if (wait_ticks == 0){
			exit_critical();
			return RET_FAILED;
		}

		block(&sq->wb_list, wait_ticks);
		wait_ticks = task_left_sleep_tick(current_tcb);
	}
}


/*
@ retv:  RET_SUCCESS / RET_FAILED.
*/
int streamq_push_isr(streamq_t sq, void *data, u_short size){
	int ret = byte_buffer_push(&sq->bbf, data, size);
	if (ret == RET_FAILED){
		return RET_FAILED;
	}
	wakeup_isr(&sq->rb_list);
	return RET_SUCCESS;
}


/*
@ brief: Read the front data of stream queue.
@ retv:  RET_FAILED -> read failed, no data now
         Other -> size of read out data
*/
int streamq_front(streamq_t sq, void *output, u_int wait_ticks){
	os_assert(lock_nesting == 0);

	enter_critical();
	while (1){
		int ret = byte_buffer_front(&sq->bbf, output);
		if (ret != RET_FAILED){
			wakeup(&sq->wb_list);
			exit_critical();
			return ret;
		}

		if (wait_ticks == 0){
			exit_critical();
			return RET_FAILED;
		}

		block(&sq->rb_list, wait_ticks);
		wait_ticks = task_left_sleep_tick(current_tcb);
	}
}


/*
@ brief: Return a pointer to the data inside the stream queue for zero-copy processing.
@ param: pointer -> Address to store the pointer to the data  
         outlen  -> Size of the data read from the buffer
@ retv:  RET_SUCCESS / RET_FAILED.
*/
int streamq_front_pointer(streamq_t sq, void **pointer, u_short *outlen, u_int wait_ticks){
	enter_critical();
	while (1){
		int ret = byte_buffer_front_pointer(&sq->bbf, pointer, outlen);
		if (ret != RET_FAILED){
			wakeup(&sq->wb_list);
			exit_critical();
			return RET_SUCCESS;
		}

		if (wait_ticks == 0){
			exit_critical();
			return RET_FAILED;
		}

		block(&sq->rb_list, wait_ticks);
		wait_ticks = task_left_sleep_tick(current_tcb);
	}
}


/*
@ brief: Pop data from stream queue.
*/
void streamq_pop(streamq_t sq){
	enter_critical();
	if (byte_buffer_pop(&sq->bbf) == RET_SUCCESS);
		wakeup(&sq->wb_list);
	exit_critical();
}

