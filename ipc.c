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
	In most cases, "signal" operation is not that important, 
	so it will not be block if exceed the maximum counting
*/

void sem_init(cntsem *s, int max_cnt, int init_cnt){
	os_assert(max_cnt >= init_cnt);

	s->max = max_cnt;
	s->count = init_cnt;
	list_init(&s->block_list);
}


cntsem* sem_create(int max_cnt, int init_cnt){
	cntsem *s = malloc(sizeof(cntsem));
	if (s == NULL){
		return NULL;
	}
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


int sem_wait(cntsem *s, u_int wait_ticks){
	enter_critical();

	while (s->count <= 0){
		if (wait_ticks == 0){
			exit_critical();
			return ERR_TIME_OUT;
		}
		block(&s->block_list, wait_ticks);
		if (s->count > 0)
			break;

		wait_ticks = get_task_left_sleep_tick(current_tcb);
	}

	s->count -= 1;
	exit_critical();
	return s->count;
}


int sem_signal(cntsem *s){
	os_assert(lock_nesting == 0);

	enter_critical();
	if (s->count >= s->max){
		exit_critical();
		return RET_FAILED;
	}
	s->count += 1;
	wakeup(&s->block_list);
	exit_critical();
	return s->count;
}


int sem_signal_isr(cntsem *s){
	if (s->count >= s->max){
		return RET_FAILED;
	}
	s->count += 1;
	wakeup_isr(&s->block_list);
	return s->count;
}


/******************************** mutex ********************************/
/*
	Mutex can handles priority inversion correctly
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
			mtx->bkp_prio = modify_priority(owner, cur_prio);

		block(&mtx->block_list, FOREVER);
	}
}


void mutex_unlock(mutex *mtx){
	os_assert(lock_nesting == 0);

	enter_critical();
	tcb_t *owner = mtx->lock_owner;
	mtx->lock_owner = NULL;

	// recover owner_task's original priority
	if (mtx->bkp_prio != 0){
		modify_priority(owner, mtx->bkp_prio);
		mtx->bkp_prio = 0;
	}

	wakeup(&mtx->block_list);
	exit_critical();
	return;
}

/******************************** message queue ********************************/
/*
	When a large amount of data needs to be transferred, 
	to avoid a large number of meaningless copies,
	threads will use a common buffer and use message queue to send the pointer to the buffer. 

	In this case, you need to wait for the queue to empty a place, then write the buffer,
	and then write the pointer to the queue.

	msgque_try_push() is used to handle this situation like this:

	int ret = msgque_try_push(queue, xticks);
	if (ret == RET_SUCCESS){
		enter_critical();
		write_buffer(buffer_point, data, size);
		msgque_overwrite(queue, buffer_point);
		exit_critical();
	}
*/

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

	queue_free(mq);
	return RET_SUCCESS;
}


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
			return ERR_TIME_OUT;
		}

		block(&mq->wb_list, wait_ticks);

		wait_ticks = get_task_left_sleep_tick(current_tcb);
	}
}


int msgq_try_push(msgque *mq, u_int wait_ticks){
	os_assert(lock_nesting == 0);

	enter_critical();
	while (1){
		if (!MSGQUE_FULL(mq)){
			exit_critical();
			return RET_SUCCESS;
		}

		if (wait_ticks == 0){
			exit_critical();
			return ERR_TIME_OUT;
		}

		block(&mq->wb_list, wait_ticks);
		
		wait_ticks = get_task_left_sleep_tick(current_tcb);
	}
}


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
@ brief: Only get the front item in queue, must use msgque_pop() after this to pop item
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
			return ERR_TIME_OUT;
		}

		block(&mq->rb_list, wait_ticks);

		wait_ticks = get_task_left_sleep_tick(current_tcb);
	}
}


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
	Use the 'value' variable of task's event_node to store bits.
	Up to support 24 bits event.
	Use the 30th bit of 'value' to store the condition (or/and).
*/

void evt_group_init(event_t grp, evt_bits_t init_bits){
	grp->evt_bits = init_bits;
	list_init(&grp->block_list);
}


event_t evt_group_create(evt_bits_t init_bits){
	event_t grp = malloc(sizeof(cntsem));
	if (grp == NULL){
		return NULL;
	}
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

	list_node_t *enode = &current_tcb->event_node;
	enode->value = (u_int)bits;
	enode->value |= (opt << 30);

	if (is_bits_satisfy(grp, enode->value) == false){
		block(&grp->block_list, wait_ticks);

		wait_ticks = get_task_left_sleep_tick(current_tcb);
		if (wait_ticks == 0){
			exit_critical();
			return ERR_TIME_OUT;
		}
	}

	if (clr == true)
		grp->evt_bits &= (~bits);

	exit_critical();
	return RET_SUCCESS;
}


void evt_set(event_t grp, evt_bits_t bits){
	enter_critical();

	grp->evt_bits |= bits;
	list_node_t *iter = &grp->block_list.dmy;

	while ((iter = iter->next) != &grp->block_list.dmy){
		if (is_bits_satisfy(grp, iter->value)){
			task_ready(EVENT_NODE_TO_TCB(iter));
			enter_critical();
		}
	}

	exit_critical();	
}


void evt_set_isr(event_t grp, evt_bits_t bits){
	grp->evt_bits |= bits;
	list_node_t *iter = &grp->block_list.dmy;

	while ((iter = iter->next) != &grp->block_list.dmy){
		if (is_bits_satisfy(grp, iter->value))
			task_ready_isr(EVENT_NODE_TO_TCB(iter));
	}
}


void evt_clear(event_t grp, evt_bits_t bits){
	enter_critical();

	grp->evt_bits &= (~bits);

	exit_critical();
}


void evt_clear_isr(event_t grp, evt_bits_t bits){
	grp->evt_bits &= (~bits);
}

