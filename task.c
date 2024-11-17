#include "inc/KoraConfig.h"

#include "inc/list.h"
#include "inc/task.h"
#include "inc/assert.h"
#include "inc/alloc.h"

#include <string.h>
#include <limits.h>

tcb_t * volatile current_tcb;

list ready_lists[CFG_MAX_PRIOS];
u_int ready_lists_prio;
list sleep_list;

extern u_int lock_nesting;
extern int switch_disable;

bool flag_actively_sched = false;
u_int lock_nesting = 0;
static int highest_prio = CFG_MAX_PRIOS-1;
static u_int os_tick_count;
static int switch_disable = 1;
int sys_task_num = 0;

// these functions defined in port.c
void start_first_task(void); 
int get_highest_priority(void);
void port_rt_stack_init(vfunc code, void *para, u_char *rt_stack);

#if CFG_DEBUG_MODE
	tcb_t *tasks_head = NULL;
#endif

/********************************** task state **********************************/

// retval: 	1: the task becomes the only and highest priority task
// 			0: highest priority has not changed 
static int add_to_ready(task_handle tsk){ 
	u_int prio = tsk->priority;
	ready_lists_prio |= 1 << prio; 
	list_insert_end(ready_lists+prio, &tsk->state_node);
	if (prio < highest_prio){
		highest_prio = prio;
		return 1;
	}
	return 0;
}


static void remove_from_ready(task_handle tsk){
	u_int prio = tsk->priority;
	if (ready_lists[prio].list_len == 1){
		ready_lists_prio &= ~(1 << prio);
	}
	list_remove(&tsk->state_node);
	highest_prio = get_highest_priority();
}


// retval: task's old priority
int modify_priority(task_handle tsk, int new){
	os_assert(new < CFG_MAX_PRIOS);

	int ret = tsk->priority;
	enter_critical();

	remove_from_ready(tsk);
	tsk->priority = new;
	add_to_ready(tsk);

	exit_critical();
	return ret;
}


extern u_int os_tick_count;
// if sleep time overflow(sleep tick + os_tick_count > UINT_MAX), reset every task's sleep time and reset tick count 
static void os_tick_reset(void){
	task_node_t *iter = sleep_list.dmy.next;
	for (int i = 0; i < sleep_list.list_len; ++i){
		iter->value -= os_tick_count;
		iter = iter->next;
	}
	os_tick_count = 0;
}


static void add_to_sleep(task_handle tsk, u_int xtick){
	if (xtick > UINT_MAX-os_tick_count)
		os_tick_reset();

	xtick += os_tick_count;
	tsk->state_node.value = xtick;
	list_insert(&sleep_list, &tsk->state_node);
}


// x -> ready
// warning: if tsk's priority higher than all other tasks
// 			this function will enter schedule immediately 
void task_ready(task_handle tsk){
	os_assert(lock_nesting == 1);
	os_assert(switch_disable == 0);

	tsk->status = ready;
	list_remove(&tsk->state_node);
	list_remove(&tsk->event_node);
	int has_changed = add_to_ready(tsk);
	exit_critical();

	
	if (has_changed){
		call_sched();
	}
}


void task_ready_isr(task_handle tsk){
	tsk->status = ready;
	list_remove(&tsk->state_node);
	list_remove(&tsk->event_node);
	int has_changed = add_to_ready(tsk);
	
	if (has_changed){
		call_sched_isr();
	}
}


// ready -> block
void block(list *blklst, u_int overtime_ticks){
	os_assert(lock_nesting == 1);
	os_assert(switch_disable == 0);

	current_tcb->status = blocking;
	if (overtime_ticks != 0){
		remove_from_ready(current_tcb);
		if (overtime_ticks != UINT_MAX)
			add_to_sleep(current_tcb, overtime_ticks);
		else
			current_tcb->state_node.value = UINT_MAX;

		list_insert_end(blklst, &current_tcb->event_node);
	}

	exit_critical();
	call_sched();

	// if block ends, code will continue from here
	enter_critical();
}


void block_isr(task_handle tsk, list *blklst, u_int overtime_ticks){
	tsk->status = blocking;
	if (overtime_ticks != 0){
		remove_from_ready(tsk);
		if (overtime_ticks != UINT_MAX)
			add_to_sleep(tsk, overtime_ticks);
		list_insert(blklst, &tsk->event_node);
	}

	call_sched_isr();
}


// ready -> suspend
void task_suspend(task_handle tsk){
	enter_critical();
	if (tsk == NULL)
		tsk = current_tcb;

	tsk->status = suspending;
	remove_from_ready(tsk);
	list_remove(&tsk->event_node);

	exit_critical();
	if (current_tcb == tsk)
		call_sched();
}


void task_suspend_isr(task_handle tsk){
	tsk->status = suspending;
	remove_from_ready(tsk);
	list_remove(&tsk->event_node);

	if (current_tcb == tsk)
		call_sched_isr();
}


// ready -> sleep
void sleep(u_int xtick){
	os_assert(switch_disable == 0);
	enter_critical();

	current_tcb->status = sleeping;
	remove_from_ready(current_tcb);
	add_to_sleep(current_tcb, xtick+1);


	exit_critical();
	call_sched();
}


u_int get_task_left_sleep_tick(task_handle tsk){
	u_int tick = tsk->state_node.value;
	if (tick == UINT_MAX)
		return UINT_MAX;

	if (tick > os_tick_count)
		return tick - os_tick_count;

	return 0;
}


task_stat get_task_state(task_handle tsk){
	return tsk->status;
}


task_handle get_running_task(void){
	return current_tcb;
}


char* get_task_name(task_handle tsk){
	if (tsk == NULL)
		tsk = current_tcb;
	return tsk->name;
}


#if CFG_DEBUG_MODE

task_handle traversing_tasks(int opt){
	static tcb_t *iter;

	if (opt == 1){
		iter = tasks_head;
		return tasks_head;
	}

	else {
		iter = iter->tcb_next;
		return iter;
	}
}

task_handle find_task(char *name){
	tcb_t *iter = tasks_head;
	while (iter != NULL){
		if (strncmp(name, iter->name, CFG_TASK_NAME_LEN) == 0)
			return iter;
	}
	return NULL;
}

#endif //CFG_DEBUG_MODE


/***************************** create and delete *****************************/

#define TCB_MAGIC_NUM 	0x0F984F1Cul
void tcb_init(tcb_t *tcb, u_int prio, const char *name, u_char *start){
	strncpy(tcb->name, name, 15);
	tcb->name[15] = 0;
	tcb->priority = prio;
	tcb->top_of_stack = (u_char*)((u_int)tcb - sizeof(u_int)*17);
	tcb->start_addr = start;
	tcb->status = ready;
	TASK_NODE_INIT(&tcb->state_node);
	TASK_NODE_INIT(&tcb->event_node);

 #if CFG_SAFETY_CHECK
	tcb->magic_n = TCB_MAGIC_NUM;
	tcb->min_stack_left = 99999;
 #endif

 #if CFG_DEBUG_MODE
	tcb->occupied_tick = 0;

	tcb_t *iter = tasks_head;
	while (iter != NULL && prio > iter->priority)
		iter = iter->tcb_next;
	tcb->tcb_next = iter->tcb_next;
	iter->tcb_next = tcb;
 #endif
}


tcb_t* task_init(vfunc code, const char *name, void *para, u_int prio, u_char *stk, int size){
	os_assert(size >= CFG_MIN_STACK_SIZE);
	os_assert(prio <= PRIORITY_LOWEST);

	static bool first_time_call = true;
	if (first_time_call){
		for (int i = 0; i < CFG_MAX_PRIOS; ++i)
			list_init(&ready_lists[i]);
		first_time_call = false;
	}
	
	u_char *stktop = (u_char*)( (u_int)stk + size);
	tcb_t *new_tcb = (tcb_t*)( (u_int)(stktop - sizeof(tcb_t)) & ALIGN4MASK);
	
	tcb_init(new_tcb, prio, name, stk);
	port_rt_stack_init(code, para, (u_char*)new_tcb);
	add_to_ready(new_tcb);
	sys_task_num += 1;

	return new_tcb;
}


tcb_t* task_create(vfunc code, const char *name, void *para, u_int prio, int size){
	u_char *stack = malloc(size);
	if (!stack)
		return NULL;
	return task_init(code, name, para, prio, stack, size);
}


// task create function with fewer paraments 
tcb_t* qcreate(vfunc code, int priority, int size){
	static char nqtask = 0;

	u_char *tstk = malloc(size);
	if (!tstk)
		return false;

	char name[] = "qtask_x";
	name[6] = nqtask + '1';
	tcb_t *ret = task_init(code, name, NULL, priority, tstk, size);
	nqtask++;
	return ret;
}


void task_delete(task_handle tsk){
	enter_critical();

	if (get_task_state(tsk) == ready)
		remove_from_ready(tsk);
	else
		list_remove(&tsk->state_node);

	list_remove(&tsk->event_node);
	if (is_heap_addr(tsk->start_addr))
		queue_free(tsk->start_addr);

	#if CFG_DEBUG_MODE
		// remove task from task linked list
		if (tasks_head == tsk){
			tasks_head = tsk->tcb_next;
		}
		else {
			tcb_t *iter = tasks_head;
			while (iter->tcb_next != tsk)
				iter = iter->tcb_next;
			iter->tcb_next = tsk->tcb_next;
		}
	#endif

	sys_task_num -= 1;

	exit_critical();
	call_sched();
}


// the task's lr register points to this function for automatic deletion at the end of the task
void task_self_delete(void){
	task_delete(current_tcb);
}


/***************************** scheduler *****************************/


// disable task switch in application layer
void disable_task_switch(void){
	++switch_disable;
}


void enable_task_switch(void){
	os_assert(switch_disable > 0);

	--switch_disable;

	if (switch_disable == 0 && highest_prio < current_tcb->priority)
		call_sched();
}


bool is_scheduler_running(void){
	return (bool)(switch_disable == 0);
}


// called in pendsv to find next task to execute
void schedule(void){
	task_node_t **it = &(ready_lists[highest_prio].iterator);
	(*it) = (*it)->next;
	if (*it == &(ready_lists[highest_prio].dmy))
		(*it) = (*it)->next;
	current_tcb = STATE_NODE_TO_TCB(*it);

 #if CFG_SAFETY_CHECK
	if (current_tcb->magic_n != TCB_MAGIC_NUM)
		while (1) ;
 #endif
}


static void wake_task_from_sleep(void){
	task_node_t *first = sleep_list.dmy.next;
	if (os_tick_count >= first->value){
		tcb_t *tcb = STATE_NODE_TO_TCB(first);
		list_remove(&tcb->event_node);
		list_remove(&tcb->state_node);
		add_to_ready(tcb);
	}
}


#if CFG_SAFETY_CHECK
	// check whether task's stack used up
	static void stack_safety_check(void){
		int free_stk_size = current_tcb->top_of_stack - current_tcb->start_addr;
		if (free_stk_size < 40)
			while (1) ;

		if (free_stk_size < current_tcb->min_stack_left)
			current_tcb->min_stack_left = free_stk_size;
	}
#endif


// If a task actively gives up CPU time by called call_sched(), 
// the handler will not trigger a task switch when the next tick arrives
void os_tick_handler(void){
	os_tick_count += 1;

 #if CFG_SAFETY_CHECK
	stack_safety_check();
 #endif

 #if CFG_DEBUG_MODE
	current_tcb->occupied_tick += 1;
 #endif
		
	if (switch_disable > 0){
		return;
	}

	if (flag_actively_sched == true){
		flag_actively_sched = false;
		return ;
	}

	if (LIST_NOT_EMPTY(&sleep_list))
		wake_task_from_sleep();
	
	if (current_tcb->priority == highest_prio && LIST_LEN(ready_lists+highest_prio) == 1)
		return;

	call_sched_isr();
}

#define CFG_IDLE_TASK_STACK_SIZE  	512
static u_char idle_stack[CFG_IDLE_TASK_STACK_SIZE];

static u_int cpu_utilization;
static u_int begin_tick = 0;

static void idle_task(void *nothing){
	extern splist *wait_for_free;

	u_int last_tick = 0, idle_tick = 0;
	
	while (1){
		while (wait_for_free){
			void *addr = wait_for_free;
			wait_for_free = wait_for_free->next;
			free(addr);
		}

		// calculate the cpu utilization
		if (last_tick != os_tick_count){
			++idle_tick;
			last_tick = os_tick_count;
		}
		if (os_tick_count - begin_tick >= 500){
			cpu_utilization = 100 - idle_tick/5;
			begin_tick = os_tick_count;
			idle_tick = 0;
		}

	}
}


// retval: 0-100
int get_cpu_util(void){
	if (os_tick_count - begin_tick > 500)
		return 100;
	return cpu_utilization;
}


int get_os_tick(void){
	return os_tick_count;
}


void Kora_start(void){
	list_init(&sleep_list);
	current_tcb = task_init(idle_task, "idle", NULL, CFG_MAX_PRIOS-1, 
			idle_stack, CFG_IDLE_TASK_STACK_SIZE);

 #if CFG_SHELL_DEBUG
	void shell_init(void);
	shell_init();
 #endif

	os_tick_count = 0;
	switch_disable = 0;
	start_first_task();
}
