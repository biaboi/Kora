#ifndef _SHELL_H
#define _SHELL_H


typedef void (*data_trans_func)(char *, int);

void shell_init(int prio, data_trans_func output);
void shell_input(char *msg, int size);

#endif
