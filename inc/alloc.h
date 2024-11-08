#ifndef _DMEM_H
#define _DMEM_H

#include "KoraConfig.h"

 #if CFG_ALLOW_DYNAMIC_ALLOC
	void* os_malloc(int size);
	void* os_calloc(int nitems, int item_size);
	void os_free(void *addr);
	void queue_free(void *addr);

	bool is_heap_addr(void *addr);
	int get_heap_remain_size(void);

	#define malloc os_malloc
	#define calloc os_calloc
	#define free   os_free
 #endif


#endif
