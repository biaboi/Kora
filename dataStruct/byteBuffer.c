#include "byteBuffer.h"
#include <string.h>


/*
 * @file    byteBuffer.c
 *
 * A circular buffer implementation for storing and retrieving byte streams in segments
 * (e.g., push strings of 30, 20, 10 bytes and pop them in the same order).
 * The buffer uses a circular queue to manage data, handling cases where the buffer
 * is split between the end and the start of the memory (i.e., when rear wraps around
 * to buffer_start while front is still ahead).
 *
 * Storage Strategy:
 * When attempting to store a new segment, the buffer first checks if there is enough
 * contiguous space between the rear pointer and buffer_end. If insufficient, the tail
 * space is discarded, and the buffer attempts to store the segment at buffer_start,
 * provided there is enough space before the front pointer. If neither space is sufficient,
 * the operation fails. This strategy prioritizes simplicity and avoids fragmentation
 * within the buffer's tail space.
 *
 */

void byte_buffer_init(byte_buffer* bbf, u_char *buf, int buf_size){
	bbf->head = buf;
	bbf->size = buf_size;
	bbf->count = 0;

	bbf->front = bbf->rear = buf;
}


/*
@ retv: The total free space of the byte buffer, including tail and head space.
*/
int byte_buffer_free_space(byte_buffer *bbf){
	if (bbf->count == 0)
		return bbf->size;

	if (bbf->front == bbf->rear)
		return 0;

	if (bbf->front > bbf->rear)
		return bbf->front - bbf->rear;
	else
		return bbf->size - (bbf->rear - bbf->front);
}


/*
@ retv: The maximum contiguous space for the next write, comparing right space
        (buffer_end - rear) and left space (front - buffer_start) in byte_buffer.
 */
int byte_buffer_max_write_space(byte_buffer *bbf){
	if (bbf->front > bbf->rear)
		return bbf->front - bbf->rear;

	u_short left_space = bbf->front - bbf->head;
	u_short right_space = bbf->head + bbf->size - bbf->rear;

	return left_space > right_space ? left_space : right_space;
}


/*
@ brief: Push data into byte buffer.
@ retv: RET_FAILED / RET_SUCCESS.
*/
int byte_buffer_push(byte_buffer *bbf, void *input, u_short len){
	if (len + 2 > byte_buffer_free_space(bbf)) 
		return RET_FAILED;

	int total_len = len + 2;

	// calculate the space of rear pointer to buffer_end (or front if wrapped)
	u_short tail_space = 0;
	if (bbf->front > bbf->rear)
		tail_space = bbf->front - bbf->rear;
	else
		tail_space = bbf->head + bbf->size - bbf->rear;


	if (tail_space >= total_len) {
		*(u_short*)(bbf->rear) = len;
		bbf->rear += 2;
		memcpy(bbf->rear, input, len);
		bbf->rear += len;
	} 

	else {
		if (bbf->front - bbf->head <= total_len) 
			return RET_FAILED;

		*(u_short*)(bbf->rear) = 0xFFFF;
		bbf->rear = bbf->head;
		*(u_short*)(bbf->rear) = len;
		bbf->rear += 2;
		memcpy(bbf->rear, input, len);
		bbf->rear += len;
	}

	bbf->count += 1;
	return RET_SUCCESS;
}


/*
@ brief: Copy data to output buffer.
@ retv:  RET_FAILED -> No data available in the buffer  
         Other -> Size of the data read from the buffer
*/
int byte_buffer_front(byte_buffer *bbf, void *output){
	if (bbf->count == 0) 
		return RET_FAILED;

	u_short len = *(u_short*)(bbf->front);
	if (len == 0xFFFF){
		bbf->front = bbf->head;
		len = *(u_short*)(bbf->front);
	}
	memcpy(output, bbf->front + 2, len);

	return len;
}


/*
@ brief: Returns a pointer to the data inside the byte_buffer for zero-copy processing.
@ param: pointer -> Address to store the pointer to the data  
         outlen  -> Size of the data read from the buffer
@ retv:  RET_SUCCESS / RET_FAILED.
*/
int byte_buffer_front_pointer(byte_buffer *bbf, void **pointer, u_short *outlen){
	if (bbf->count == 0){
		*pointer = NULL;
		*outlen = 0;
		return RET_FAILED;
	} 

	u_short len = *(u_short*)(bbf->front);
	if (len == 0xFFFF){
		bbf->front = bbf->head;
		len = *(u_short*)(bbf->front);
	}
	*pointer = bbf->front + 2;
	*(u_short*)outlen = len;

	return RET_SUCCESS;
}


/*
@ brief: Pop data in byte_buffer. 
*/
int byte_buffer_pop(byte_buffer *bbf){
	if (bbf->count == 0)
		return RET_FAILED;

	u_short len = *(u_short*)(bbf->front);
	if (len == 0xFFFF){
		bbf->front = bbf->head;
		len = *(u_short*)(bbf->front);
	}
	bbf->front += len + 2;
	bbf->count -= 1;

	return RET_SUCCESS;
}
