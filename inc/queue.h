#ifndef _QUEUE_H
#define _QUEUE_H

#include "KoraDef.h"

typedef struct queue {
	void   *head;
	short   item_size;
	short   max;

	short   front;
	short   rear;

	int     len;
} queue;


#define QUEUE_IS_FULL(que)   ((que)->len >= (que)->max)
#define QUEUE_IS_EMPTY(que)  ((que)->len == 0)
#define QUEUE_LEN(que)       ((que)->len)

void queue_init(queue *que, void *buf, int nitems, int size);
int queue_push(queue *que, void *item);
int queue_front(queue *que, void *buf);
int queue_pop(queue *que, void *buf);



#endif
