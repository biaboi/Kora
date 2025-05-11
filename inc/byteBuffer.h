#ifndef _BYTE_BUFFER
#define _BYTE_BUFFER

#include "KoraDef.h"

typedef struct {
	u_char   *head;
	u_short   size;   // size of buffer
	u_short   count;  // number of data 

	u_char   *front;
	u_char   *rear;
} byte_buffer;

#define BYTE_BUFFER_COUNT(bbf)  ((bbf)->count)


void byte_buffer_init(byte_buffer* bbf, u_char *buf, int buf_size);
int byte_buffer_free_space(byte_buffer *bbf);
int byte_buffer_push(byte_buffer *bbf, void *input, u_short len);
int byte_buffer_front(byte_buffer *bbf, void *output);
int byte_buffer_front_pointer(byte_buffer *bbf, void **pointer, int *outlen);
void byte_buffer_pop(byte_buffer *bbf);

#endif
