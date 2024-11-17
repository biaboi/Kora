#include "KoraConfig.h"
#include "ipc.h"
#include "string.h"

#if CFG_SHELL_DEBUG 
 #if CFG_DEBUG_MODE == 0
  #error "only shell in debug mode"
 #endif


int get_task_info(task_handle tsk, char *buf);


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

command commands[] = { {"task", __task}, 
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


void shell_input(char *msg){
	strncpy(input_buf, msg, 40);
	input_buf[39] = 0;
	evt_set(&shell_cmd_recv_evt, CMD_RECV_EVT);
}


void shell_output(char* buf, int size){

}


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
	if (strncmp(tokens[1], "all", CFG_TASK_NAME_LEN)){
		return RET_FAILED;	
	}

	task_handle the_task = find_task(tokens[1]);
	if (the_task != NULL){
		int size = get_task_info(the_task, output_buf);
		shell_output(output_buf, size);
		return RET_SUCCESS;
	}

	return RET_FAILED;
}

/**************************** shell commands end ****************************/


char* ui_to_s(int n, char *str) {
    int i = 0, j = 0;
    do {
        str[i++] = n%10 + '0';
        n /= 10;
    } while(n);
    str[i] = '\0';

    while (j < i/2) {
        str[j] 	   ^=  str[i-1-j];
        str[i-1-j] ^=  str[j];
        str[j]     ^=  str[i-1-j];
        ++j;
    } 
    return str;
}


static char *stat_to_str[5] = {
	"running", "ready", "sleeping", "blocking", "suspending",
};


// get these info: name, priority, status, occupied_tick, min_stack_left
int get_task_info(task_handle tsk, char *buf){
	char *write_in = buf;
	strncat(write_in, tsk->name, CFG_TASK_NAME_LEN);
	write_in += CFG_TASK_NAME_LEN + 4;

	ui_to_s(tsk->priority, write_in);
	write_in += 4;

	strncat(write_in, stat_to_str[(int)(tsk->status)], 12);
	write_in += 12;

 #if CFG_DEBUG_MODE
	ui_to_s(tsk->occupied_tick, write_in);
	write_in += 10;
 #endif

 #if CFG_SAFETY_CHECK
	ui_to_s(tsk->min_stack_left, write_in);
	write_in += 8;
 #endif

	return (int)(write_in - buf);
}


#endif // CFG_SHELL_DEBUG

