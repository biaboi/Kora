#include "KoraConfig.h"
#include "ipc.h"
#include "string.h"
#include "stdio.h"
#include "port.h"
#include "alloc.h"

#if CFG_SHELL_DEBUG 

#define NL  "\r\n"

int set_task_table_title(void);
int get_task_info(task_handle tsk);
int setheap_info_table_title(void);
int output_heap_status(void);

static char input_buf[40];
static char output_buf[100];
evt_group shell_cmd_recv_evt; 
static char *tokens[6];

#define CMD_RECV_EVT  1


typedef struct {
    const char *cmd;
    int (*handler)(void);
} command;


int __task(void);
int __heap(void);

command commands[] = { {"task", __task}, 
					   {"heap", __heap}, 
};

#define commands_size (sizeof(commands) / sizeof(command))


static int shell_parse_cmd(void){
	int i = 0;
	char *token = strtok(input_buf, " ");
	while (token != NULL){
		tokens[i++] = token;
		if (token == NULL)
			break;
		token = strtok(NULL, " ");
	}
	return i;
}


static int shell_exec(char * cmd){
	for (int i = 0; i < commands_size; ++i){
		if (strcmp(cmd, commands[i].cmd) == 0){
			return (commands[i].handler)();
		}
	}

	return RET_FAILED;
}


__weak void shell_input(char *msg, int size){
	strncpy(input_buf, msg, size);
	input_buf[39] = 0;
	evt_set_isr(&shell_cmd_recv_evt, CMD_RECV_EVT);
}


void shell_output(char* buf, int size);


void shell_task(void *nothing){
	while (1){
		evt_wait(&shell_cmd_recv_evt, CMD_RECV_EVT, 1, 1, FOREVER);

		int narg = shell_parse_cmd();
		if (narg == 0)
			continue;

		shell_exec(tokens[0]);
	}
}


static u_char shell_task_stack[1100];

void shell_init(void){
	evt_group_init(&shell_cmd_recv_evt, 0);
	task_init(shell_task, "shell", NULL, CFG_SHELL_TASK_PRIO,
		shell_task_stack, 1100);
}


/****************************** shell commands ******************************/

static int __task(void) {
	int size;

	if (strncmp(tokens[1], "all", CFG_TASK_NAME_LEN) == 0){
		size = set_task_table_title();
		shell_output(output_buf, size);

		task_handle iter = traversing_tasks(1);
		size = get_task_info(iter);
		shell_output(output_buf, size);
		
		while ((iter = traversing_tasks(0)) != NULL){
			size = get_task_info(iter);
			shell_output(output_buf, size);
		}

		shell_output(NL, 2);
		return RET_SUCCESS;
	}

	task_handle the_task = find_task(tokens[1]);
	if (the_task != NULL){
		size = set_task_table_title();
		shell_output(output_buf, size);

		size = get_task_info(the_task);
		shell_output(output_buf, size);
		shell_output(NL, 2);
		return RET_SUCCESS;
	}

	return RET_FAILED;
}


#if CFG_ALLOW_DYNAMIC_ALLOC
	static int __heap(void){
		int size;

		size = setheap_info_table_title();
		shell_output(output_buf, size);

		size = output_heap_status();
		shell_output(output_buf, size);
		
		return RET_SUCCESS;
	}

#else
	static int __heap(void){
		return RET_FAILED;
	}

#endif


/**************************** shell commands end ****************************/

static char *stat_to_str[5] = {
	"running", "ready", "sleep", "block", "suspend",
};


static int set_task_table_title(void){
	return sprintf(output_buf, "%-10s  %6s  %8s   %6s   %9s \r\n", 
				"name",
				"prio",
				"status",
				"min_stack",
				"cpu_usage" );
}

// get these info: name, priority, status, min_stack, occupied_tick
int get_task_info(task_handle tsk){
	int usage = get_os_tick() / 100;
	usage = tsk->occupied_tick / usage;

	return sprintf(output_buf, "%-10s  %6d  %8s   %4d   %9d \r\n",  
				tsk->name, 
				tsk->priority, 
				stat_to_str[(int)(tsk->status)], 
				tsk->min_stack,
				usage );
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


static int output_heap_status(void){
	heap_info_t *info = get_heap_status();
	return sprintf(output_buf, "%6d %10d %10d %14d %15d %12d \r\n",  
				info->remain_size,
				info->malloc_count,
				info->free_count,
				info->peak_heap_usage,
				info->max_free_block_size,
				info->left_block_num  );
}


#endif // CFG_SHELL_DEBUG

