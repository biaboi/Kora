#ifndef _PORT_H
#define _PORT_H

#include "prjdef.h"

void enter_critical(void);
void exit_critical(void);
int get_highest_priority(void);
void start_first_task(void);

#define NVIC_ICSR_REG 		(*((volatile u_int *)0xe000ed04))
#define NVIC_PENDSV_SET 	(u_int)(1u << 28)
#define call_sched_isr()  	(NVIC_ICSR_REG |= NVIC_PENDSV_SET)
#define call_sched() 		do { extern bool flag_actively_sched; \
								 flag_actively_sched = true; \
								 __asm ("svc #1"), \
							} while (0);

// #define NVIC_SHPR3_REG		(*(volatile u_int*)0xe000ed20)
// #define PENDSV_PRIORITY 		((u_int)(0xEFu << 16))   // =14
// #define SYSTICK_PRIORITY 	((u_int)(0xEFu << 24))   // =14

#define os_tick_handler 	SysTick_Handler

#define rt_stk_init    		port_stack_init
void port_stack_init(vfunc code, void *para, u_char *rt_stack);

void enable_os_tick(void);

#endif
