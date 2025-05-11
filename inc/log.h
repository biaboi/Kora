#ifndef _LOG_H
#define _LOG_H

#include "Kora.h"
#include <string.h>
#include <time.h>

#define LOG_OUTPUT_MODE_DMA    0
#define LOG_OUTPUT_MODE_BLOCK  1

enum {log_mode_block, log_mode_dma};

typedef enum {
	lev_debug = 0, lev_info, lev_warn, lev_error, lev_fatal
} log_level;


typedef struct {
	char     name[16];
	int      output_level;
	char     onoff;
	char     reserved;
	u_short  err_count;    // record level >= error counts
} log_module;

typedef log_module* log_module_t;

void log_module_init(log_module *module, char *name, log_level out_lev);
log_module_t log_module_create(char *name, log_level out_lev);

void module_on(log_module_t module);
void module_off(log_module_t module);

void os_log(log_module_t module, log_level level, const char *fmt, ...);
void log_dma_ready_notify(void);
void log_set_output(transfer_t output, int mode);
void log_set_level(log_module_t module, log_level lev);
task_handle log_system_init(int task_prio, transfer_t out, int mode);

#define log_debug(module, fmt, ...)  os_log(module, lev_debug, fmt, __VA_ARGS__)
#define log_info(module, fmt, ...)   os_log(module, lev_info,  fmt, __VA_ARGS__)
#define log_warn(module, fmt, ...)   os_log(module, lev_warn,  fmt, __VA_ARGS__)
#define log_error(module, fmt, ...)  os_log(module, lev_error, fmt, __VA_ARGS__)
#define log_fatal(module, fmt, ...)  os_log(module, lev_fatal, fmt, __VA_ARGS__)

#endif
