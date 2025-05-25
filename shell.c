/*
 * Kora rtos
 * Copyright (c) 2024 biaboi
 *
 * This file is part of this project and is licensed under the MIT License.
 * See the LICENSE file in the project root for full license information.
 */

#include "Kora.h"
#include "KoraConfig.h"
#include "log.h"
#include "shell.h"

#include "stdio.h"
#include "string.h"
#include "stdlib.h"

/*
	Command List:

	1. task      - Task management commands
	   -a        : Display information for all tasks
	   -s        : Suspend a task
	   -r        : Resume a task
	   -i <name> : Display information for the specified task

	2. heap      - Display current heap usage and state

	3. log       - Logging control commands
	   <name> <op>
	     op:
	       on/off               : Enable or disable the specified module
	       debug/info/warn/error: Set the output log level for the module
*/


#define INPUT_BUF_SIZE   40

static u_char   task_stack[1200];
static char     in_buf[INPUT_BUF_SIZE];
static char     out_buf[100];
static char    *tokens[10];

static evt_group     recv_evtgrp; 
static transfer_t    output = NULL;

typedef struct {
    char  name[16];
    cmd_handler  handler;
} command_t;



int __task(int argc, char **agrv);
int __heap(int argc, char **agrv);
int __log(int argc, char **agrv);
int __var(int argc, char **argv);

static shell_var_t var_table[MAX_NUM_OF_EXPORT_VAR];
static u_char num_of_var = 0;

static command_t commands[10] = {
	{"task", __task}, 
	{"heap", __heap},  
	{"log", __log},
	{"var", __var},
};
static u_char commands_size = 4;


void shell_export_var(char* name, void* addr, var_type_t type) {
	os_assert(num_of_var < MAX_NUM_OF_EXPORT_VAR);

	var_table[num_of_var] = (shell_var_t){"", addr, type};
	strncpy(var_table[num_of_var].name, name, EXPORT_VAR_NAME_LEN);
	++num_of_var;
}


/*
@ brief: Register a command to the system.
*/
void shell_register_command(char *name, cmd_handler handler){
	os_assert(commands_size < 10);

	command_t *cur = commands + commands_size;
	strncpy(cur->name, name, 16);
	cur->name[15] = 0;
	cur->handler = handler;
	++commands_size;
}


/*
@ brief: Parse input command and execute.
@ retv: RET_SUCCESS / RET_FAILED.
*/
static int parse_and_execute(void){
	int valid = 0;
	char *token = strtok(in_buf, " ");
	while (token != NULL){
		tokens[valid++] = token;
		token = strtok(NULL, " ");
	}

	if (valid == 0)
		return RET_FAILED;

	for (int i = 0; i < commands_size; ++i){
		if (strncmp(tokens[0], commands[i].name, 16) == 0){
			int ret = (commands[i].handler)(valid - 1, tokens + 1);
			return ret;
		}
	}

	return RET_FAILED;
}


/*
@ brief: Copy intput data to buffer and wake up shell_task.
*/
void shell_input(char *msg, int size){
	strncpy(in_buf, msg, size);
	in_buf[INPUT_BUF_SIZE-1] = 0;
	evt_set_isr(&recv_evtgrp, 1);
}


void shell_task(void *nothing){
	while (1){
		evt_wait(&recv_evtgrp, 1, 1, 1, FOREVER);

		parse_and_execute();
	}
}


/*
@ brief: Init shell system.
@ param: Prio -> shell_task's priority.
         Out -> shell output function.
*/
void shell_init(int prio, transfer_t out){
	output = out;
	evt_group_init(&recv_evtgrp, 0);
	task_init(shell_task, "shell", NULL, prio, task_stack, 1200);
}


void name_combine(int argc, char **argv, char *out);
int read_hex(const char *hex, u_char *out);

/*************************** build-in command: var ***************************/
static char *var_type_to_str[6] = {
	"hex", "int", "uint", "float", "bool", "string"
};

#define OUT_STR_MAX_LEN   32

static int sprint_var_data(char *buf, shell_var_t *desc){
	char outlen = 0;
	int str_len = 0;

	switch (desc->type){
	case var_hex:
		outlen = sprintf(buf, "%X"NL, *(u_char*)(desc->addr));
		break;

	case var_int:
		outlen = sprintf(buf, "%d"NL, *(int*)(desc->addr));
		break;

	case var_uint:
		outlen = sprintf(buf, "%u"NL, *(int*)(desc->addr));
		break;

	case var_float:
		outlen = sprintf(buf, "%f"NL, *(float*)(desc->addr));
		break;

	case var_bool:
		strcpy(buf, *(char*)(desc->addr) ? "true "NL : "false"NL);
		outlen = 7;
		break;

	case var_string:
		str_len = strlen(desc->addr);
		if (str_len >= OUT_STR_MAX_LEN-3){
			strncpy(buf, (char *)(desc->addr), OUT_STR_MAX_LEN - 6);
			strcat(buf, "..."NL);
			outlen = OUT_STR_MAX_LEN;
		}
		else {
			strcpy(buf, (char *)(desc->addr));
			buf[++str_len] = '\r';
			buf[++str_len] = '\n';
			buf[++str_len] = 0;
			outlen = str_len;
		}

		break;
	}
	return outlen;
}


static void set_var_data(shell_var_t *desc, char *raw){
	switch (desc->type) {
	case var_hex:
		if (read_hex(raw, desc->addr) == RET_FAILED)
			output("wrong hex format"NL, 19);
		break;

	case var_int:
	case var_uint:
		*(int*)(desc->addr) = atoi(raw);
		break;

	case var_float:
		*(float*)(desc->addr) = atof(raw);
		break;

	case var_bool:
		if (strcmp(raw, "true") == 0 || strcmp(raw, "1") == 0)
			*(u_char*)(desc->addr) = 1;
		else if (strcmp(raw, "false") == 0 || strcmp(raw, "0") == 0)
			*(u_char*)(desc->addr) = 0;
		else
			output("wrong bool format"NL, 20);
		break;

	case var_string:
		strncpy(desc->addr, raw, OUT_STR_MAX_LEN);
		break;
	}

}


int __var(int argc, char **argv){
	int len;
	shell_var_t *desc = NULL;

	if (strncmp(argv[0], "list", 4) == 0){
		output("  name           type     value"NL, 34);
		for (int i = 0; i < num_of_var; ++i) {
			shell_var_t *desc = var_table+i;
			len = sprintf(out_buf, " %-16s %-6s    ", desc->name, var_type_to_str[desc->type]);
			output(out_buf, len);

			len = sprint_var_data(out_buf, desc);
			output(out_buf, len);
		}
		return RET_SUCCESS;
	}

	for (int i = 0; i < num_of_var; ++i) {
		char *name = var_table[i].name;
		if (strncmp(argv[0], name, EXPORT_VAR_NAME_LEN) == 0){
			desc = var_table+i;
			break;
		}
	}
	if (desc == NULL){
		output("unknown variable"NL, 19);
		return RET_FAILED;
	}


	if (argc == 1){
		int len = sprint_var_data(out_buf, desc);
		output(out_buf, len);
		return RET_SUCCESS;
	}

	if (argv[1][0] != '='){
		output("incorrect format"NL, 19);
		return RET_FAILED;
	}

	set_var_data(desc, argv[2]);

	return RET_SUCCESS;
}


/*************************** build-in command: task ***************************/
static char *stat_to_str[5] = {
	"run", "ready", "sleep", "block", "susp",
};


static int set_task_table_title(void){
	return sprintf(out_buf, "%-10s  %6s  %8s   %6s   %9s \r\n", 
				"name", "prio", "state", "min_stack", "cpu_usage" );
}


/*
@ brief: Write task's infomations to buffer and send: 
         name, priority, state, min_stack, occupied_tick
*/
void output_task_info(task_handle tsk, void *nothing){
	int usage = os_get_tick() / 100;
	usage = tsk->occupied_tick / usage;

	int size = sprintf(out_buf, "%-10s  %6d  %8s   %4d   %9d "NL,  
				tsk->name, 
				tsk->priority, 
				stat_to_str[(int)(tsk->state)], 
				tsk->min_stack,
				usage );
	output(out_buf, size);
}


int __task(int argc, char **argv) {
	char buf[CFG_TASK_NAME_LEN];
	int size;   // output data size

	// output all task's infomation
	if (strncmp(argv[0], "-a", CFG_TASK_NAME_LEN) == 0){
		size = set_task_table_title();
		output(out_buf, size);

		foreach_task(output_task_info, NULL);
		output(NL, sizeof(NL));
		return RET_SUCCESS;
	}

	// suspend a task
	else if (strncmp(argv[1], "-s", CFG_TASK_NAME_LEN) == 0){
		name_combine(argc-1, argv+1, buf);
		task_handle the_task = task_find(buf);
		if (the_task == NULL){
			output("task do not exist"NL, 20);
			return RET_FAILED;
		}
		else if (task_state(the_task) != suspend)
			task_suspend(the_task);
	}


	// resume a task
	else if (strncmp(argv[0], "-r", CFG_TASK_NAME_LEN) == 0){
		name_combine(argc-1, argv+1, buf);
		task_handle the_task = task_find(buf);
		if (the_task == NULL){
			output("task do not exist"NL, 20);
			return RET_FAILED;
		}
		else if (task_state(the_task) > ready){
			enter_critical();
			task_ready(the_task);
		}
	}


	// output information about the specified task
	else if (strncmp(argv[0], "-i", CFG_TASK_NAME_LEN) == 0) {
		name_combine(argc-1, argv+1, buf);
		task_handle the_task = task_find(buf);
		if (the_task == NULL){
			output("task do not exist"NL, 20);
			return RET_FAILED;
		}
		else {
			size = set_task_table_title();
			output(out_buf, size);

			output_task_info(the_task, NULL);
			return RET_SUCCESS;
		}
	}

	else {
		output("unknown parameter"NL, 20);
		return RET_FAILED;
	}

	return RET_FAILED;
}

/*************************** build-in command: heap ***************************/
static int setheap_info_table_title(void){
	return sprintf(out_buf, "%8s %8s %8s %8s %12s\r\n", 
				"usage", "alloced", "freed", "peak", "freg size");
}


static int output_heap_state(void){
	heap_status_t *info = heap_status();
	u_int remain = info->remain_size;

	char usage_percent = (CFG_HEAP_SIZE - remain)*100 / CFG_HEAP_SIZE;

	return sprintf(out_buf, "%6d/%d%% %6d  %7d  %7d %10d\r\n",  
				CFG_HEAP_SIZE - remain,
				usage_percent,
				info->malloc_count,
				info->free_count,
				info->peak_heap_usage,
				info->left_block_num);
}


int __heap(int argc, char **agrv){
#if CFG_ALLOW_DYNAMIC_ALLOC
	int size;

	size = setheap_info_table_title();
	output(out_buf, size);

	size = output_heap_state();
	output(out_buf, size);
	
	return RET_SUCCESS;

#else
	return RET_FAILED;

#endif
}


/**************************** build-in command: log ****************************/

int __log(int argc, char **argv){
	log_module_t module = module_find(argv[0]);
	if (module == NULL){
		output("module do not exist"NL, 22);
		return RET_FAILED;
	}

	if (strncmp(argv[1], "on", 8) == 0)
		module->onoff = 1;
	else if (strncmp(argv[1], "off", 8) == 0)
		module->onoff = 0;
	else if (strncmp(argv[1], "debug", 8) == 0)
		module->output_level = lev_debug;
	else if (strncmp(argv[1], "info", 8) == 0)
		module->output_level = lev_info;
	else if (strncmp(argv[1], "warn", 8) == 0)
		module->output_level = lev_warn;
	else if (strncmp(argv[1], "error", 8) == 0)
		module->output_level = lev_error;
	else {
		output("unknown parameter"NL, 20);
		return RET_FAILED;
	}

	return RET_SUCCESS;
}

/************************************ misc ************************************/

static void name_combine(int argc, char **argv, char *out) {
    if (argc <= 1 )
        return;

    out[0] = '\0';

    for (int i = 0; i < argc; ++i) {
        strcat(out, argv[i]);
        if (i != argc - 1) {
            strcat(out, " ");
        }
    }
}


/*
@ brief: Read a 2byte hex data.
@ retv: RET_SUCCESS / RET_FAILED.
*/
static int read_hex(const char *hex, u_char *out) {
    if (!hex || !out)
        return RET_FAILED;

    if ((hex[0] == '0') && (hex[1] == 'x' || hex[1] == 'X'))
        hex += 2;

    u_char value = 0;

    for (int i = 0; i < 2; ++i) {
        char c = hex[i];
        if (c >= '0' && c <= '9')
            value = (value << 4) | (c - '0');
        else if (c >= 'A' && c <= 'F')
            value = (value << 4) | (c - 'A' + 10);
        else if (c >= 'a' && c <= 'f') 
            value = (value << 4) | (c - 'a' + 10);
        else
            return RET_FAILED;
    }

    *out = value;
    return RET_SUCCESS;
}
