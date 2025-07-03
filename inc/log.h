#ifndef _LOG_H
#define _LOG_H

#include "Kora.h"
#include <string.h>
#include <time.h>

#define NL  "\r\n"
#define CFG_MAX_NUM_OF_LOG_MODULE   8

typedef enum {
	lev_debug = 0, lev_info, lev_warn, lev_error, lev_fatal
} log_level;

typedef void (*log_callback)(const char* log_msg, log_level level, void* udata);

typedef struct {
	char    name[16];
	log_level   output_level;

	volatile bool  is_enabled;        // whether this module working
	volatile bool  is_include_name;   // whether log message contains the module name

	log_callback   user_callback;
	void* udata;   // user-defined data
} log_module;

typedef log_module* log_module_t;


typedef struct {
	bool    is_timestamp;
	const char* (*get_time_str)(void);
	log_callback   callback;
} log_config_t;


void log_module_init(log_module *module, char *name, log_level out_lev);
log_module_t log_module_create(char *name, log_level out_lev);

void module_set_callback(log_module_t module, log_callback ucb, void *user_data);
log_module_t module_find(char *name);
void foreach_log_module(void (*process)(log_module_t module));

void log_init(transfer_t out_func, log_callback callback);
void log_set_timefunc(const char* (*pfunc)(void));
void log_use_timestamp(bool state);
void os_log(log_module_t module, log_level level, const char *fmt, ...);

#define MODULE_ON(module)   ((module)->is_enabled = true)
#define MODULE_OFF(module)  ((module)->is_enabled = false)
#define MODULE_INC_NAME(module)      ((module)->is_include_name = true)
#define MODULE_NOT_INC_NAME(module)  ((module)->is_include_name = false)
#define MODULE_SET_LEVEL(module, lev)  ((module)->output_level = (lev))

#if CFG_USING_LOG_SYSTEM
	#define log_debug(module, ...)  os_log(module, lev_debug, __VA_ARGS__)
	#define log_info(module, ...)   os_log(module, lev_info,  __VA_ARGS__)
	#define log_warn(module, ...)   os_log(module, lev_warn,  __VA_ARGS__)
	#define log_error(module, ...)  os_log(module, lev_error, __VA_ARGS__)
	#define log_fatal(module, ...)  os_log(module, lev_fatal, __VA_ARGS__)
#else
	#define log_debug(module, ...)  (void)0
	#define log_info(module, ...)   (void)0
	#define log_warn(module, ...)   (void)0
	#define log_error(module, ...)  (void)0
	#define log_fatal(module, ...)  (void)0
#endif

#endif
