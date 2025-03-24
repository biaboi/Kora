#ifndef _SHELL_H
#define _SHELL_H


typedef void (*transfer_t)(char *, int);

void shell_init(int prio, transfer_t output);
void shell_input(char *msg, int size);

#endif
