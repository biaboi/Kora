#ifndef _PIPE_H
#define _PIPE_H

#include "Kora.h"

typedef enum {DROP = 0, BLOCK} pipe_overflow_policy;

typedef struct {
	task_handle  work_tsk;
	transfer_t   process;   // user-defined process fucntion
	streamq_t    sq;
	sem_t        ready_sem;

	volatile char  state;
	volatile bool  is_full;
	pipe_overflow_policy    policy;
} pipe_t;


pipe_t* pipe_create(transfer_t cb, char *sq_buf, int sq_buf_size);
void pipe_set_overf_policy(pipe_t *pip, pipe_overflow_policy poli);
task_handle pipe_start(pipe_t *p, char *name, u_int prio, char *stack);
void pipe_stop(pipe_t *p);
int pipe_push(pipe_t *p, char *data, int len);
void pipe_ack(pipe_t *pip);
void pipe_ack_isr(pipe_t *pip);
void pipe_flush(pipe_t *p);

#endif
