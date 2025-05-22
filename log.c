/*
 * Kora rtos
 * Copyright (c) 2024 biaboi
 *
 * This file is part of this project and is licensed under the MIT License.
 * See the LICENSE file in the project root for full license information.
 */


#include "log.h"
#include "KoraConfig.h"

#include <stdio.h>
#include <stdarg.h>

/**
 * @file    log.c
 * @brief   Logging system implementation for user space.
 *
 * This file provides a logging system designed for user.
 * It utilizes a stream queue to buffer and transmit log messages efficiently.
 * The system supports DMA-based transmission for improved performance.
 * Logging modules are categorized by tags to differentiate various subsystems.
 */


/**
 * @note for log_task: 
 * 
 * Logging task using zero-copy stream queue for efficiency.
 *
 * This task retrieves log entries from a stream queue using zero-copy,
 * meaning it directly uses pointers to the internal buffer of the stream queue
 * rather than copying the data into a local buffer. This improves performance
 * and reduces memory overhead.
 *
 * However, this approach introduces a critical risk: if streamq_pop() is called
 * immediately after initiating a DMA transmission (e.g., via UART), the memory
 * pointed to by the queue entry may still be in use by the DMA hardware. If
 * new log data is pushed into the queue before the DMA transfer completes,
 * it can overwrite the memory region still being used by the DMA, leading to
 * data corruption, misordered output, or garbled logs.
 *
 */

#define LOG_TASK_STACK_SIZE   512

#if CFG_USING_LOG_SYSTEM

static sem_t   dma_sem;
static char    log_task_stack[LOG_TASK_STACK_SIZE];

static streamq  log_que;
static char     log_buffer[CFG_LOG_BUFFER_SIZE];

static int           cur_mode;
static transfer_t    log_output;

static int num_of_modules = 0;
static log_module_t  all_modules[CFG_MAX_NUM_OF_MODULE];

static const char *level_str[5] = {
	"debug", "info", "warn", "error", "fatal"
};


void log_module_init(log_module *module, char *name, log_level out_lev){
	os_assert(num_of_modules < CFG_MAX_NUM_OF_MODULE);
	
	strncpy(module->name, name, 16);
	module->name[15] = 0;
	module->output_level = out_lev;
	module->onoff = 1;
	module->fatal_callback = NULL;
	all_modules[num_of_modules++] = module;
}


#if CFG_ALLOW_DYNAMIC_ALLOC
/*
@ brief: Create a module, default enable.
*/
log_module_t log_module_create(char *name, log_level out_lev){
	log_module *module = malloc(sizeof(log_module));
	if (module == NULL)
		return NULL;

	log_module_init(module, name, out_lev);
	return module;
}
#endif


void module_on(log_module_t module){
	module->onoff = 1;
}


void module_off(log_module_t module){
	module->onoff = 0;
}


void module_set_level(log_module_t module, log_level lev){
	module->output_level = lev;
}


void module_set_callback(log_module_t module, vfunc cb){
	module->fatal_callback = cb;
}


log_module_t module_find(char *name){
	for (int i = 0; i < num_of_modules; ++i)
		if (strncmp(all_modules[i]->name, name, 16) == 0)
			return all_modules[i];
	return NULL;
}

void log_set_output(transfer_t output, int mode){
	log_output = output;
	cur_mode = mode;
}


/*
@ brief: Process log and send to log_task.
@ note: The size limit of a single log is 128.
*/
void os_log(log_module_t module, log_level level, const char *fmt, ...){
	if (module->onoff == 0 || level < module->output_level)
		return ;

	char buffer[128];
	va_list args;

	int offset = snprintf(buffer, 28, "<%s>[%s]", module->name, level_str[level]);

	va_start(args, fmt);
	offset += vsnprintf(buffer + offset, 100 - offset, fmt, args);
	va_end(args);

	if (level == lev_fatal && module->fatal_callback != NULL)
		(module->fatal_callback)(NULL);

	u_int wait;
	if (level >= lev_warn)
		wait = FOREVER;
	else
		wait = 100;

	streamq_push(&log_que, buffer, offset, wait);    
}


/*
@ brief: Print through log system's dma(if dma_mode) channel.
@ retv: The size of the successfully output data.
*/
int log_printf(const char *fmt, ...){
	char buffer[128];

	va_list args;
	va_start(args, fmt);
	int offset = vsnprintf(buffer, 128, fmt, args);
	va_end(args);

	streamq_push(&log_que, buffer, offset, FOREVER);
	return offset;
}


/*
@ brief: Output data through log system's dma(if dma_mode) channel.
*/
void log_puts(char *data, int size){
	streamq_push(&log_que, data, size, FOREVER);
}


/*
@ brief: Log system main task.
@ note:
*/
static void log_task(void *nothing){
	char *str;
	u_short len = 0;

	while (1){
		int ret = streamq_front_pointer(&log_que, (void**)&str, &len, 10000);
		if (ret != RET_SUCCESS)
			continue;

		if (cur_mode == log_mode_dma){
			ret = sem_wait(dma_sem, 1000);
			if (ret != RET_SUCCESS)
				continue;
		}

		log_output(str, len);
		streamq_pop(&log_que);

	/*
		char buf[128];
		memcpy(buf, str, len);
		log_output(buf, len);
		streamq_pop(&log_que);
	*/
	}
}


/*
@ brief: Create a log task to process logs. 
@ param: Task_Prio -> log_task's priority, depend on transmit function is in
                      DMA mode or block mode, the former use higher priority
                      and the latter use lower priority
         Out -> output function
         S -> If using DMA to transfer data, pass in the semaphore that controls the DMA, 
              otherwise pass in NULL.
         Mode -> transmit function whether using DMA 
@ retv: Log_task's handle.
*/
task_handle log_system_init(int task_prio, transfer_t out, sem_t s, int mode){
	dma_sem = s;
	sem_init(dma_sem, 1, 1);
	streamq_init(&log_que, log_buffer, CFG_LOG_BUFFER_SIZE);

	log_output = out;
	cur_mode = mode;

	return task_init(log_task, "log task", NULL, task_prio, 
		            (u_char*)log_task_stack, LOG_TASK_STACK_SIZE);
}

#endif // CFG_USING_LOG_SYSTEM
