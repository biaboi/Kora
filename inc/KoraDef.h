#ifndef _MISC_H
#define _MISC_H

#include <stddef.h>

typedef char bool;
#define true 1
#define false 0
// #define NULL ((void*)0)


#define ALIGN4MASK 	0xFFFFFFFC
#define ALIGN8MASK 	0xFFFFFFF8

#define RET_SUCCESS   		(int)0
#define RET_FAILED   		(int)(-1)

typedef unsigned char  u_char;
typedef unsigned short u_short;
typedef unsigned int   u_int;

typedef void (*vfunc)(void*);

typedef struct simple_list {
	struct simple_list *next;
} splist;



#endif
