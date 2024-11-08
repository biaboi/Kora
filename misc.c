#include "inc/KoraConfig.h"

#include "inc/task.h"
#include "inc/assert.h"

/**************************** assert ******************************/
#if CFG_KORA_ASSERT

struct assert_info {
	char *file_name;
	int line;
} assert_info;


void os_assert_failed(char *file_name, int line){
	assert_info.file_name = file_name;
	assert_info.line = line;
	while (1);
}

#endif

/*************************** profiler *****************************/

#if CFG_ENABLE_PROFILER

// check whether task's stack used up
void stack_safety_check(void){
	extern tcb_t *current_tcb;

	int free_stk_size = current_tcb->top_of_stack - current_tcb->start_addr;
	os_assert(free_stk_size > 0);

	if (free_stk_size < current_tcb->min_stack_left)
		current_tcb->min_stack_left = free_stk_size;
}

// cpu utilization: percent of idle task runtime in last 1 sec
// calculate in systick_handler
int cpu_utilization;

int get_cpu_util(void){
	return cpu_utilization;
}

#endif

/************************** os service ****************************/

void os_service(char No){
	if (No == 1){
		call_sched_isr();
	}
}


