#include "list.h"
#include "queue.h"
#include <string.h>

// circular queue

void queue_init(queue *que, void *buf, int nitems, int size){
	que->head = buf;
	que->item_size = size;
	que->max = nitems;

	que->front = que->rear = que->len = 0;
}

// if queue is full, the item at the front of queue will be overwritten
// retval:queue's length
int queue_push(queue *que, void *item){
	if (QUEUE_IS_FULL(que))
		que->front = (que->front+1) % que->max;
	else
		que->len += 1;

	int size = que->item_size;
	void *des = (void*)( (u_int)(que->head) + size*que->rear);
	memcpy(des, item, size);
	que->rear = (que->rear + 1) % que->max;

	return que->len;
}


int queue_front(queue *que, void *buf){
	if (QUEUE_IS_EMPTY(que))
		return RET_FAILED;

	int size = que->item_size;
	void *src = (void*)( (u_int)(que->head) + size*que->front);
	memcpy(buf, src, size);
	return que->len;
}


int queue_pop(queue *que, void *buf){
	int ret = queue_front(que, buf);
	if (ret == RET_SUCCESS){
		que->front = (que->front + 1) % que->max;
		que->len -= 1;
	}
	return que->len;
}

