#include "KoraConfig.h"

#include "task.h"
#include "assert.h"

#include "string.h"

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


/************************** os service ****************************/

void os_service(char No){
	if (No == 1){
		call_sched_isr();
	}
}





