/*
 * Kora rtos
 * Copyright (c) 2024 biaboi
 *
 * This file is part of this project and is licensed under the MIT License.
 * See the LICENSE file in the project root for full license information.
 */


#include "log.h"

#include <stdio.h>
#include <stdarg.h>

/**
 * @file    log.c
 * @brief   Logging system implementation for user space.
 *
 * This file provides the implementation of a flexible logging system designed for user-space applications.
 * The system supports the following features:
 * - Optional timestamp inclusion in log messages for precise event tracking.
 * - Optional module name prefixing to identify the source of log messages.
 * - Multi-level callback mechanism for customizable log handling.
 * - Configurable log levels and module-specific filtering to control log output verbosity.
 *
 * The logging system is designed to be lightweight and extensible, suitable for embedded or resource-constrained environments.
 * Warning: Log module only allow to dynamic allocation, don't support dynamic free.
 */


#define INNER_BUF_SIZE  140

static transfer_t  log_output = NULL;
static log_callback  common_cb = NULL;

static log_config_t lsys = {false, NULL};

static int num_of_modules = 0;
static log_module_t  all_modules[CFG_MAX_NUM_OF_LOG_MODULE];

const char *level_str[5] = {
	"debug", "info", "warn", "error", "fatal"
};


/*
@ brief: Initialize the log_module structure.
*/
void log_module_init(log_module *module, char *name, log_level out_lev){
	os_assert(num_of_modules < CFG_MAX_NUM_OF_LOG_MODULE);
	
	strncpy(module->name, name, 16);
	module->name[15] = 0;
	module->output_level = out_lev;

	module->is_enabled = true;
	module->is_include_name = true;

	module->user_callback = NULL;
	module->udata = NULL;

	enter_critical();
	all_modules[num_of_modules++] = module;
	exit_critical();
}


/*
@ brief: Create a module and enable.
*/
log_module_t log_module_create(char *name, log_level out_lev){
	log_module *module = malloc(sizeof(log_module));
	if (module == NULL)
		return NULL;

	log_module_init(module, name, out_lev);
	return module;
}


void module_set_callback(log_module_t module, log_callback ucb, void *user_data){
	module->user_callback = ucb;
	module->udata = user_data;
}


log_module_t module_find(char *name){
	for (int i = 0; i < num_of_modules; ++i)
		if (strncmp(all_modules[i]->name, name, 16) == 0)
			return all_modules[i];
	return NULL;
}


void foreach_log_module(void (*process)(log_module_t module)){
	for (int i = 0; i < num_of_modules; ++i)
		process(all_modules[i]);
}


void log_set_timefunc(const char* (*pfunc)(void)){
	lsys.is_timestamp = true;
	lsys.get_time_str = pfunc;
}


void log_use_timestamp(bool state){
	lsys.is_timestamp = state;
}


void log_init(transfer_t out_func, log_callback callback){
	log_output = out_func;
	common_cb = callback;
}

/*
@ brief: Process log message.
@ note: Log function use log_output for explicit output like uart, screen...
		If user want to storage log message, use user_callback.
@ warning: The size limit of a single log is INNER_BUF_SIZE.
*/
void os_log(log_module_t module, log_level level, const char *fmt, ...){
	os_assert(log_output != NULL);

	if (module->is_enabled == 0 || level < module->output_level)
		return ;

	char buffer[INNER_BUF_SIZE];
	int offset = 0;

	// formatting log message prefix
	if (lsys.is_timestamp)
		offset += snprintf(buffer, 40, "[%s]", lsys.get_time_str());

	if (module->is_include_name)
		offset += snprintf(buffer + offset, 20, "<%s>", module->name);

	offset += snprintf(buffer + offset, 10, "[%s]", level_str[level]);


	// formatting log message body
	va_list args;
	va_start(args, fmt);
	offset += vsnprintf(buffer + offset, INNER_BUF_SIZE - offset, fmt, args);
	va_end(args);

	log_output(buffer, offset);

	if (module->user_callback != NULL)
		module->user_callback(buffer, level, module->udata);

	if (common_cb != NULL)
		common_cb(buffer, level, NULL);
}
