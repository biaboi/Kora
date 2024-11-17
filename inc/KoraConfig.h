#ifndef	_UNVS_CONFIG_H
#define _UNVS_CONFIG_H


#define CFG_CPU_CLOCK_HZ			168000000
#define CFG_TICK_PER_SEC			1000
#define CFG_TASK_NAME_LEN 			16

#define CFG_ALLOW_DYNAMIC_ALLOC  	1
#define CFG_KORA_HEAP_SIZE 			(u_int)(20 * 1024)


// if 1, task control block will add a field called status
// if 0, the status is determined by the list where the state_node and event_node resides
#define CFG_DEBUG_MODE		 		1

#define CFG_SAFETY_CHECK	 		1

#define CFG_KORA_ASSERT  			1

#define CFG_SHELL_DEBUG 			1
#define CFG_SHELL_TASK_PRIO 		6

#define CFG_MAX_PRIOS				16

#define CFG_MIN_STACK_SIZE  		256

#include "port.h"
#include "KoraDef.h"

#endif
