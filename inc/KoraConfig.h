#ifndef	_UNVS_CONFIG_H
#define _UNVS_CONFIG_H

#include "port.h"

#define CFG_ALLOW_DYNAMIC_ALLOC  	1
#define CFG_KORA_HEAP_SIZE 			(u_int)(20 * 1024)

#define CFG_ENABLE_PROFILER 		1

#define CFG_KORA_ASSERT  			1

#define CFG_MAX_PRIOS				16

#define CFG_MIN_STACK_SIZE  		256   // contain tcb's size

#define CFG_IDLE_TASK_STACK_SIZE  	512

#endif
