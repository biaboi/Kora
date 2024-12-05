#ifndef	_UNVS_CONFIG_H
#define _UNVS_CONFIG_H


#define CFG_CPU_CLOCK_HZ            72000000
#define CFG_TICK_PER_SEC            1000
#define CFG_TASK_NAME_LEN           16
#define CFG_MAX_PRIOS               16
#define CFG_MIN_STACK_SIZE          400


#define CFG_ALLOW_DYNAMIC_ALLOC     1
#define CFG_HEAP_SIZE               (u_int)(20 * 1024)

#define CFG_KORA_ASSERT             1

#define CFG_USE_LOG		            0


#include "port.h"

#endif
