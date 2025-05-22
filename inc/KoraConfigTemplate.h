#ifndef	_KORA_CONFIG_H
#define _KORA_CONFIG_H


#define CFG_CPU_CLOCK_HZ            64000000
#define CFG_TICK_PER_SEC            1000
#define CFG_TASK_NAME_LEN           16
#define CFG_MAX_PRIOS               16
#define CFG_MIN_STACK_SIZE          400


#define CFG_ALLOW_DYNAMIC_ALLOC     1
#define CFG_HEAP_SIZE               (u_int)(20 * 1024)

#define CFG_KORA_ASSERT             1

#define CFG_USING_LOG_SYSTEM        1
#define CFG_LOG_BUFFER_SIZE         400
#define CFG_MAX_NUM_OF_MODULE       8

#define CFG_USE_KERNEL_HOOKS        1
#define CFG_USE_ALLOC_HOOKS         1
#define CFG_USE_IPC_HOOKS           1


#define NL  "\n"

#include "port.h"

#endif
