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



#if CFG_USING_LOG_SYSTEM

/*
	This file provides a log system for kernel and user.
	Feature: 1. designed for the rtos and DMA transmit
	         2. module + level, flexible management of log output
	         3. optional timestamp
*/

static cntsem  output_dma_sem;
static char    log_task_stack[1024];

static streamq log_que;
static char    log_buffer[CFG_LOG_BUFFER_SIZE];

static int         cur_mode;
static transfer_t  log_output;

static const char *level_str[5] = {
	"debug", "info", "warn", "error", "fatal"
};


void log_module_init(log_module *module, char *name, log_level out_lev){
	strncpy(module->name, name, 16);
	module->name[15] = 0;
	module->output_level = out_lev;
	module->onoff = 1;
	module->err_count = 0;
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


/*
@ brief: Use this function to notice log_task that DMA transmit complete.
@ note: This function must be called in ISR.
*/
void log_dma_ready_notify(void){
	sem_signal_isr(&output_dma_sem);
}


void log_set_output(transfer_t output, int mode){
	log_output = output;
	cur_mode = mode;
}


void log_set_level(log_module_t module, log_level lev){
	module->output_level = lev;
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

	int offset = snprintf(buffer, 128, "<%s>[%s]", module->name, level_str[level]);

	va_start(args, fmt);
	offset += vsnprintf(buffer + offset, 128 - offset, fmt, args);
	va_end(args);

	u_int wait;
	if (level >= lev_warn)
		wait = FOREVER;
	else
		wait = 1000;

	streamq_push(&log_que, buffer, offset, wait);    
}


static void log_task(void *nothing){
	char *str;
	int len = 0;

	while (1){
		streamq_front_pointer(&log_que, (void**)&str, &len, FOREVER);
		if (cur_mode == log_mode_dma)
			sem_wait(&output_dma_sem, 1000);
		log_output(str, len);
		streamq_pop(&log_que);
	}
}


/*
@ brief: Create a log task to process logs. 
@ param: Task_Prio -> log_task's priority, depend on transmit function is in
                      DMA mode or block mode, the former use higher priority
                      and the latter use lower priority
         Mode -> transmit function whether using DMA 
@ retv: Log_task's handle.
*/
task_handle log_system_init(int task_prio, transfer_t out, int mode){
	task_handle h;
	sem_init(&output_dma_sem, 1, 1);
	streamq_init(&log_que, log_buffer, CFG_LOG_BUFFER_SIZE);
	h = task_init(log_task, "log task", NULL, task_prio, (u_char*)log_task_stack, 1024);

	log_output = out;
	cur_mode = mode;

	return h;
}

#endif // CFG_USING_LOG_SYSTEM
