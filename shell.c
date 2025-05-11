/*
 * Kora rtos
 * Copyright (c) 2024 biaboi
 *
 * This file is part of this project and is licensed under the MIT License.
 * See the LICENSE file in the project root for full license information.
 */

#include "Kora.h"
#include "shell.h"
#include "stdio.h"
#include "string.h"


#define NL  "\r\n"

#define RECV_EVT_BIT  1

static u_char   task_stack[1100];
static char     input_buf[40];
static char     output_buf[100];
static char    *tokens[6];
static evt_group     recv_evtgrp; 
static transfer_t    output = NULL;


typedef struct {
    const char *cmd;
    int (*handler)(void);
} command_t;


/*********************************  commands declare *********************************/

int __task(void);
int __heap(void);

command_t commands[] = { {"task", __task}, 
					     {"heap", __heap},  };

#define commands_size (sizeof(commands) / sizeof(command_t))


/**************************** parse and execute commands ****************************/

/*
@ brief: Parse input command and save tokens to tokens[6];
@ retv: Valid number of tokens
*/
static int parse_cmd(void){
	int valid = 0;
	char *token = strtok(input_buf, " ");
	while (token != NULL){
		tokens[valid++] = token;
		if (token == NULL)
			break;

		token = strtok(NULL, " ");
	}
	return valid;
}


/*
@ brief: Execute command
*/
static int exec_cmd(char * cmd){
	for (int i = 0; i < commands_size; ++i){
		if (strcmp(cmd, commands[i].cmd) == 0){
			return (commands[i].handler)();
		}
	}

	return RET_FAILED;
}



/*
@ brief: Copy intput data to buffer and wake up shell_task
*/
void shell_input(char *msg, int size){
	strncpy(input_buf, msg, size);
	input_buf[39] = 0;
	evt_set_isr(&recv_evtgrp, RECV_EVT_BIT);
}


void shell_task(void *nothing){
	while (1){
		evt_wait(&recv_evtgrp, RECV_EVT_BIT, 1, 1, FOREVER);

		int narg = parse_cmd();
		if (narg == 0)
			continue;

		exec_cmd(tokens[0]);
	}
}


void shell_init(int prio, transfer_t output_func){
	output = output_func;
	evt_group_init(&recv_evtgrp, 0);
	task_init(shell_task, "shell", NULL, prio, task_stack, 1100);
}


/****************************** shell commands ******************************/

int set_task_table_title(void);
int setheap_info_table_title(void);
int output_heap_state(void);
void output_task_info(task_handle tsk, void *nothing);

static int __task(void) {
	int size;

	// output all task's infomation
	if (strncmp(tokens[1], "-a", CFG_TASK_NAME_LEN) == 0){
		size = set_task_table_title();
		output(output_buf, size);

		foreach_task(output_task_info, NULL);

		output(NL, 2);
		return RET_SUCCESS;
	}

	// suspend a task
	else if (strncmp(tokens[1], "-s", CFG_TASK_NAME_LEN) == 0){
		task_handle the_task = task_find(tokens[2]);
		if (the_task == NULL){
			output("task do not exist\r\n", 20);
			return RET_FAILED;
		}
		else if (task_state(the_task) != suspend)
			task_suspend(the_task);
	}


	// resume a task
	else if (strncmp(tokens[1], "-r", CFG_TASK_NAME_LEN) == 0){
		task_handle the_task = task_find(tokens[2]);
		if (the_task == NULL){
			output("task do not exist\r\n", 20);
			return RET_FAILED;
		}
		else if (task_state(the_task) > ready){
			enter_critical();
			task_ready(the_task);
		}
	}


	// output information about the specified task
	else if (strncmp(tokens[1], "-i", CFG_TASK_NAME_LEN) == 0) {
		task_handle the_task = task_find(tokens[2]);
		if (the_task == NULL){
			output("task do not exist\r\n", 20);
			return RET_FAILED;
		}
		else {
			size = set_task_table_title();
			output(output_buf, size);

			output_task_info(the_task, NULL);
			output(NL, 2);
			return RET_SUCCESS;
		}
	}

	else {
		output("unknown parameter\r\n", 20);
		return RET_FAILED;
	}

	return RET_FAILED;
}


#if CFG_ALLOW_DYNAMIC_ALLOC
	static int __heap(void){
		int size;

		size = setheap_info_table_title();
		output(output_buf, size);

		size = output_heap_state();
		output(output_buf, size);
		
		return RET_SUCCESS;
	}

#else
	static int __heap(void){
		return RET_FAILED;
	}

#endif

/**************************** shell commands end ****************************/

static char *stat_to_str[5] = {
	"run", "ready", "sleep", "block", "susp",
};


static int set_task_table_title(void){
	return sprintf(output_buf, "%-10s  %6s  %8s   %6s   %9s \r\n", 
				"name",
				"prio",
				"state",
				"min_stack",
				"cpu_usage" );
}


/*
@ brief: Write task's infomations to buffer and send: name, priority, state, min_stack, occupied_tick
*/
void output_task_info(task_handle tsk, void *nothing){
	int usage = os_get_tick() / 100;
	usage = tsk->occupied_tick / usage;

	int size = sprintf(output_buf, "%-10s  %6d  %8s   %4d   %9d \r\n",  
				tsk->name, 
				tsk->priority, 
				stat_to_str[(int)(tsk->state)], 
				tsk->min_stack,
				usage );
	output(output_buf, size);
}


static int setheap_info_table_title(void){
	return sprintf(output_buf, "%6s %10s %10s %14s %15s %12s \r\n", 
				"remain",
				"alloced",
				"freed",
				"peak usage",
				"biggst block",
				"freg size"  );
}


static int output_heap_state(void){
	heap_info_t *info = get_heap_status();
	return sprintf(output_buf, "%6d %10d %10d %14d %15d %12d \r\n",  
				info->remain_size,
				info->malloc_count,
				info->free_count,
				info->peak_heap_usage,
				info->max_free_block_size,
				info->left_block_num  );
}



