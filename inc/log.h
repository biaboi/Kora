#ifndef _LOG_H
#define _LOG_H

#include "Kora.h"
#include <string.h>
#include <time.h>

enum {log_mode_block, log_mode_dma};

typedef enum {
	lev_debug = 0, lev_info, lev_warn, lev_error, lev_fatal
} log_level;

typedef struct {
	char    name[16];
	short   output_level;
	short   onoff;
	vfunc   fatal_callback;
} log_module;

typedef log_module* log_module_t;


void log_module_init(log_module *module, char *name, log_level out_lev);
log_module_t log_module_create(char *name, log_level out_lev);

void module_on(log_module_t module);
void module_off(log_module_t module);
void module_set_callback(log_module_t module, vfunc cb);
void module_set_level(log_module_t module, log_level lev);
log_module_t module_find(char *name);

void os_log(log_module_t module, log_level level, const char *fmt, ...);
void log_set_output(transfer_t output, int mode);
task_handle log_system_init(int task_prio, transfer_t out, sem_t s, int mode);


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
