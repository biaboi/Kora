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

typedef enum {
	hook_alloc_failed = 0,   // param: pass in the size of os_malloc()
    hook_free_failed,        // param: pass in the address of os_free()

    alloc_hook_nums
} alloc_hooks_t;


#if CFG_USE_ALLOC_HOOKS
    extern   vfunc alloc_hooks[alloc_hook_nums];
    #define  KERNEL_HOOK_ADD(x, func)  (kernel_hooks[x] = (func))
    #define  KERNEL_HOOK_DEL(x)        (kernel_hooks[x] = NULL)
#else
    #define  KERNEL_HOOK_ADD(x, func)  ((void)0)
    #define  KERNEL_HOOK_DEL(x)        ((void)0)
#endif


#endif // CFG_ALLOW_DYNAMIC_ALLOC


#endif // _ALLOC_H
