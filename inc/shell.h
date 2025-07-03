#ifndef _SHELL_H
#define _SHELL_H

#define EXPORT_VAR_NAME_LEN    16
#define MAX_NUM_OF_EXPORT_VAR  20

typedef int (*cmd_handler)(int argc, char *agrv[]);

typedef enum {var_hex, var_int, var_uint, var_float, var_bool, var_string} var_type_t;

typedef struct {
	char   name[EXPORT_VAR_NAME_LEN];
	void  *addr;
	int    type;
} shell_var_t;

void shell_register_command(char *name, cmd_handler handler);
void shell_export_var(char *name, void *addr, var_type_t type);
char* shell_wait_response(void);

void shell_init(int prio, transfer_t output);
void shell_input_isr(char *msg, int size);

#define SHELL_EXPORT_VAR(var, type) shell_export_var(#var, &(var), var_##type)

#endif
