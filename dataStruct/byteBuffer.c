#include "byteBuffer.h"
#include <string.h>


void byte_buffer_init(byte_buffer* bbf, u_char *buf, int buf_size){
	bbf->head = buf;
	bbf->size = buf_size;
	bbf->count = 0;

	bbf->front = bbf->rear = buf;
}


static int tail_space(byte_buffer *bbf) {
    if (bbf->front > bbf->rear)
        return bbf->front - bbf->rear;
    else
        return bbf->head + bbf->size - bbf->rear;
}


int byte_buffer_free_space(byte_buffer *bbf){
	if (bbf->count == 0)
		return bbf->size;

    if (bbf->front > bbf->rear)
        return bbf->front - bbf->rear;
    else
        return bbf->size - (bbf->rear - bbf->front);
}


/*
@ brief: Push data into byte buffer.
@ retv: RET_FAILED / RET_SUCCESS.
*/
int byte_buffer_push(byte_buffer *bbf, void *input, u_short len){
	if (len + 2 > byte_buffer_free_space(bbf)) 
		return RET_FAILED;

	int total_len = len + 2;

	if (tail_space(bbf) >= total_len) {
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


int byte_buffer_push_string(byte_buffer *bbf, char *str){
	int len = strlen(str);
	return byte_buffer_push(bbf, str, len+1);
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
void byte_buffer_pop(byte_buffer *bbf){
	if (bbf->count == 0)
		return;

	u_short len = *(u_short*)(bbf->front);
	if (len == 0xFFFF){
		bbf->front = bbf->head;
		len = *(u_short*)(bbf->front);
	}
	bbf->front += len + 2;
	bbf->count -= 1;
}
