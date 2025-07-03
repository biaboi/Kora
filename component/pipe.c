/*
 * Kora rtos
 * Copyright (c) 2024 biaboi
 *
 * This file is part of this project and is licensed under the MIT License.
 * See the LICENSE file in the project root for full license information.
 */


#include "Kora.h"
#include "pipe.h"
#include <string.h>

/*
	This file provides the implementation of the pipe_t abstraction for inter-task 
	stream processing in RTOS environment.
	
	A pipe_t acts as a dedicated task that continuously reads data from a stream queue 
	and processes it using a user-defined callback.
	
	This mechanism enables safe, flexible, and extensible data forwarding,
	suitable for shared hardware resources like UART DMA logging or other 
	stream-based data process.
*/

#define PIPE_TASK_STACK_LEN   1400

enum {waiting = 0, processing};

/*
@ brief: Task for processing pipe data.
@ note:  The acknowledge mechanism is implemented using a semaphore (sem_t).
@ warning: This function assumes that the ready_sem is initially available.
 */
static void pipe_task(void *_pip) {
	pipe_t *pip = (pipe_t *)_pip;
	if (!pip || !pip->sq || !pip->process)
		return;

	u_char  *data;
	u_short size;

	cntsem  sem;
	sem_init(&sem, 1, 1);
	pip->ready_sem = &sem;

	while (1){
		pip->state = waiting;
		streamq_front_pointer(pip->sq, (void**)&data, &size, FOREVER);

		pip->state = processing;
		pip->process((char*)data, size);

		sem_wait(&sem, FOREVER);
		streamq_pop(pip->sq);
	}
}


/*
@ brief: Initialize an instance of pipe_t.
@ warning: Size of sq_buf must contain the space of stream queue header.
*/
pipe_t* pipe_create(transfer_t cb, char *sq_buf, int sq_buf_size){
	pipe_t *pip = malloc(sizeof(pipe_t));

	pip->process = cb;

	streamq_t sq = (streamq_t)sq_buf;
	streamq_init(sq, sq + sizeof(streamq), sq_buf_size - sizeof(streamq));

	pip->sq = sq;

	pip->work_tsk = NULL;
	return pip;
}


void pipe_set_overf_policy(pipe_t *pip, pipe_overflow_policy poli){
	pip->policy = poli;
}


/*
@ brief: Run pipe_t task.
@ param: Name -> name of task.
         Stack -> running stack of task, the size of stack must be PIPE_TASK_STACK_LEN, use dynamic alloc if NULL. 
*/
task_handle pipe_start(pipe_t *p, char *name, u_int prio, char *stack){
	task_handle hd;
	if (stack != NULL)
		hd = task_init(pipe_task, name, p, prio, (u_char*)stack, PIPE_TASK_STACK_LEN);
	else
		hd = task_create(pipe_task, name, p, prio, PIPE_TASK_STACK_LEN);

	p->work_tsk = hd;

	return hd;
}


void pipe_suspend(pipe_t *pip){
	task_suspend(pip->work_tsk);
}


void pipe_resume(pipe_t *pip){
	enter_critical();
	task_ready(task_self());
}


void pipe_stop(pipe_t *p){
	if (is_heap_addr(p->work_tsk))
		task_delete(p->work_tsk);
	p->work_tsk = NULL;
	p->ready_sem = NULL;
}


/*
@ brief: Push data into the pipe, the specific behaviors will be affected by pipe_overflow_policy.
@ retv: RET_SUCCESS / RET_FAILED.
*/
int pipe_push(pipe_t *pip, char *data, int len){
	int ret = streamq_push(pip->sq, data, len, 0);
	if (ret == RET_SUCCESS)
		return ret;

	if (pip->policy == DROP)
		return RET_FAILED;

	return streamq_push(pip->sq, data, len, FOREVER);
}


/*
@ brief: Notifies the pipe that the current data has been fully processed,
         allowing it to take out and process the next data segment.
*/
void pipe_ack(pipe_t *pip){
	sem_signal(pip->ready_sem);
}


void pipe_ack_isr(pipe_t *pip){
	sem_signal_isr(pip->ready_sem);
}


/*
@ brief: Wait for the pipe completely processed all buffered data.
*/
void pipe_flush(pipe_t *p){
	while (p->sq->bbf.count != 0){
		sleep(10);
	}
}
