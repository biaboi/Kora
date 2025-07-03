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
} heap_status_t;


void* os_malloc(int size);
void* os_calloc(int nitems, int item_size);
void os_free(void *addr);
void queue_free(void *addr);

bool is_heap_addr(void *addr);
int get_heap_remain_size(void);
heap_status_t* heap_status(void);

#define malloc os_malloc
#define calloc os_calloc
#define free   os_free

typedef enum {
	hook_alloc_failed = 0,   // para: the size of os_malloc()
    hook_free_failed,        // para: the address of os_free()

    alloc_hook_nums
} alloc_hooks_t;


#if CFG_USE_ALLOC_HOOKS
    extern   vfunc alloc_hooks[alloc_hook_nums];
    #define  ALLOC_HOOK_ADD(x, func)  (alloc_hooks[x] = (func))
    #define  ALLOC_HOOK_DEL(x)        (alloc_hooks[x] = NULL)
#else
    #define  ALLOC_HOOK_ADD(x, func)  ((void)0)
    #define  ALLOC_HOOK_DEL(x)        ((void)0)
#endif


#endif // CFG_ALLOW_DYNAMIC_ALLOC


#endif // _ALLOC_H
