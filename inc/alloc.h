#ifndef _ALLOC_H
#define _ALLOC_H

#include "KoraConfig.h"
#include "Kora.h"

#if CFG_ALLOW_DYNAMIC_ALLOC

typedef struct {
	u_int   remain_size;
	short   malloc_count;
	short   free_count;
	int     peak_heap_usage; 
	int     max_free_block_size;
	int     left_block_num;
} heap_info_t;


void* os_malloc(int size);
void* os_calloc(int nitems, int item_size);
void os_free(void *addr);
void queue_free(void *addr);

bool is_heap_addr(void *addr);
int get_heap_remain_size(void);
heap_info_t* get_heap_status(void);

#define malloc os_malloc
#define calloc os_calloc
#define free   os_free


#endif // CFG_ALLOW_DYNAMIC_ALLOC


#endif // _ALLOC_H
