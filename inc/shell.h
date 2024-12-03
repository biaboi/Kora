#ifndef _SHELL_H
#define _SHELL_H


typedef void (*)(char *, int) data_trans_func;
void shell_init(int prio, data_trans_func output);

#endif
