#include "main.h"
#include "KoraConfig.h"

#define EXCRET_MSP_HANDLE		0xFFFFFFF1
#define EXCRET_MSP_THREAD		0xFFFFFFF9
#define EXCRET_PSP_THREAD		0xFFFFFFFD

typedef unsigned int u_int;

extern u_int lock_nesting;

// disable all interrpution except NMI and HardFault
void enter_critical(void){
	__asm {
		cpsid i;
		dsb
		isb
	}
	lock_nesting += 1;
}


void exit_critical(void){
	if (--lock_nesting == 0){
		__asm {
			cpsie i;
		}
	}
} 


void task_self_delete(void);
void port_rt_stack_init(vfunc code, void *para, u_char *rt_stack){
	u_int *pt = (u_int*)rt_stack;
	*--pt = (u_int)0x01000000;   		// xpsr
	*--pt = (u_int)code;				// pc
	*--pt = (u_int)task_self_delete;	// lr
	pt -= 4;
	*--pt = (u_int)para;				// r0
	*--pt = EXCRET_PSP_THREAD;   	// lr in handler
}


__asm int get_highest_priority(void){
	extern ready_lists_prio
	PRESERVE8

	ldr  	r0, =ready_lists_prio
	ldr  	r0, [r0]
	rbit 	r1, r0
	clz 	r0, r1
	bx		lr
	nop
}


__asm void enable_vfp(void){
	PRESERVE8

    ldr.w	r0, =0xE000ED88
    ldr		r1, [r0]

    orr		r1, r1, #(0xf << 20)
    str		r1, [r0]
    bx		r14
    nop
}


// when entry a pendsv exception, xpsr, pc, lr, r12 and r0 ~ r3 will 
// be automatically pushed, pendsv_handler will manually push r4 ~ r11,
// and save psp to the first member of tcb.
__asm void PendSV_Handler(void){
	extern current_tcb
	extern schedule
	PRESERVE8

	cpsid   i
	mrs 	r0, psp	 				// sp point to top of stack (r0)
	isb		

	ldr  	r2, =current_tcb		// r2 = &current_tcb
	ldr  	r1, [r2] 

	tst 	r14, #0x10				// check if previous context used fpu, if true, stack fpu registers
	it 		eq
	vstmdbeq 	r0!, {s16 - s31}	// save floating point registers

	stmdb  	r0!, {r4 - r11, r14}
	str  	r0, [r1] 				// save new sp to the first member of tcb
	/* manually push end */

	push 	{r2, lr}				// handler using msp, call function will change
									// the value of r2 and lr, need to protect 
	dsb
	isb
	bl  	schedule
	pop 	{r2, lr}

	ldr  	r1, [r2]
	ldr  	r0, [r1] 				// r0 = top_of_stack
	ldmia	r0!, {r4 - r11, r14}

	tst 	r14, #0x10
	it 		eq
	vldmiaeq 	r0!, {s16 - s31}

	msr 	psp, r0

	cpsie   i
	isb
	bx  	lr
	nop
}


/***************************** SVC_Handler *****************************/

__asm void SVC_Handler(void){
	extern current_tcb
	extern os_service 
	PRESERVE8

	tst    	lr, #0x4 
	ite    	eq 
	mrseq  	r0, msp 
	mrsne  	r0, psp 

	ldr  	r1, [r0, #24]
	ldrsb 	r0, [r1, #-2]
	/* get svc number and save in r0 */

	cbnz  	r0, mysvc

	ldr 	r2, =current_tcb
	ldr 	r1, [r2]
	ldr 	r0, [r1]
	ldmia 	r0!, {r4-r11, r14}
	msr 	psp, r0
	isb
	mov 	r0, #0  			// use isb to ensure system is using psp now
	msr 	basepri, r0
	orr 	lr, #0xd
	bx 		lr
	/* use svc_handler to start first task and switch msp to psp */

mysvc
	push	{lr}
	bl		os_service  
	pop 	{lr}
	bx 		lr
	nop
}

#define NVIC_VTOR_REG   	0xE000ED08

// After reset, system enters the privileged level and use msp, to switch to unprivileged level
// and use psp, need to set correlation registers in isr functions. 
static __asm void _start_first_task(void){
	PRESERVE8
	ldr 	r0, =NVIC_VTOR_REG
	ldr 	r0, [r0]
	ldr 	r0, [r0]		// get the init value of msp 
	msr 	msp, r0			
	cpsie 	i
	cpsie 	f
	dsb
	isb						// ensure exception enable before svc call
	svc 	#0
}

#define FPCCR	(*(volatile u_int*)0xE000EF34)   // floating point context control register
#define ASPEN_AND_LSPEN_BITS   ( 0x3u << 30u )

#define NVIC_SHPR3_REG		(*(volatile u_int*)0xE000ED20)
#define PENDSV_PRIORITY 	((u_int)(0xFFul << 16))		// =15
#define SYSTICK_PRIORITY 	((u_int)(0xFFul << 24))		// =15

void start_first_task(void){
	lock_nesting = 0;

	enable_vfp();
	FPCCR |= ASPEN_AND_LSPEN_BITS;  // use lazy stack

	NVIC_SHPR3_REG |= PENDSV_PRIORITY;
	NVIC_SHPR3_REG |= SYSTICK_PRIORITY;

	SysTick_Config(CFG_CPU_CLOCK_HZ / CFG_TICK_PER_SEC);  // declared in core_cm4.h
	SysTick->CTRL |= 0x1;  // enable systick
	_start_first_task();
}

