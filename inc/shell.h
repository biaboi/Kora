#ifndef _SHELL_H
#define _SHELL_H

typedef int (*cmd_handler)(int argc, char *agrv[]);

void shell_register_command(char *name, cmd_handler handler);
void shell_init(int prio, transfer_t output);
void shell_input(char *msg, int size);

#endif
