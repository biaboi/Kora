#ifndef _PORT_H
#define _PORT_H


#include "KoraDef.h"

void enter_critical(void);
void exit_critical(void);

#define NVIC_ICSR_REG 		(*((volatile u_int *)0xE000ED04))
#define NVIC_PENDSV_SET 	(u_int)(1u << 28)
#define call_sched_isr()  	(NVIC_ICSR_REG |= NVIC_PENDSV_SET)
#define call_sched() 		do { extern bool flag_actively_sched; \
								 flag_actively_sched = true; \
								 __asm ("svc #1"), \
							} while (0);

#define os_tick_handler 	SysTick_Handler

#define IS_IN_IRQ()             (__get_IPSR_s() != 0U)


#endif
