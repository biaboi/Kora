#ifndef _QUEUE_H
#define _QUEUE_H

#include "KoraDef.h"

typedef struct {
	void    *buf;
	u_short  item_size;
	u_short  max;

	u_short  front;
	u_short  rear;

	u_short  len;
	u_short  reserved;
} queue;


#define QUEUE_IS_FULL(que)   ((que)->len >= (que)->max)
#define QUEUE_IS_EMPTY(que)  ((que)->len == 0)
#define QUEUE_LEN(que)       ((que)->len)

void queue_init(queue *que, void *buf, int nitems, int size);
int queue_push(queue *que, void *item);
int queue_front(queue *que, void *buf);
int queue_pop(queue *que, void *buf);



#endif
