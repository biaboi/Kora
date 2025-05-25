/*
 * Kora rtos
 * Copyright (c) 2024 biaboi
 *
 * This file is part of this project and is licensed under the MIT License.
 * See the LICENSE file in the project root for full license information.
 */

#include "KoraConfig.h"
#include "Kora.h"
#include <string.h>

#if CFG_ALLOW_DYNAMIC_ALLOC


#if CFG_USE_ALLOC_HOOKS
	vfunc alloc_hooks[alloc_hook_nums] = { NULL };
	#define EXECUTE_HOOK(x, para) do{ if (alloc_hooks[x]) (alloc_hooks[x])(para);} while (0)
#else
	#define EXECUTE_HOOK(x, para) ((void)0)
#endif



static heap_status_t info = {CFG_HEAP_SIZE}; 
static int min_left = CFG_HEAP_SIZE;


static u_char unaligned_heap[CFG_HEAP_SIZE];
static u_char *heap = unaligned_heap;

// header of the allocated memory block
typedef struct header {
	u_short size;
	u_short magic;
} header_t;

// header of the unallocated memory block
typedef struct block{
	u_int size;
	struct block *next;
} block_t;


static block_t *end;
static block_t *iter;


static void heap_init(void){
	int remainder = (u_int)unaligned_heap & ~ALIGN4MASK;
	if (remainder != 0){
		heap += 4 - remainder; 
		info.remain_size -= 4 - remainder;
	}

	block_t *first = (block_t*)heap;
	first->size = CFG_HEAP_SIZE;
	first->next = first;

	end = first;
	iter = first;
}



/*
@ brief: Dynamic allocation function, use next fit algorithm to find free block.
*/
void* os_malloc(int size){
	static bool first_time_call = true;

	disable_task_switch();

	if (first_time_call) {
		heap_init();
		first_time_call = false;
	}

	// means that the current heap size is insufficient
	if (size == 0 || size + sizeof(header_t) >= info.remain_size){
		EXECUTE_HOOK(hook_alloc_failed, &size);
		enable_task_switch();
		return NULL;
	}

	// keep size a multiple of 4
	if (size < sizeof(block_t)) 
		size = sizeof(block_t);
	int val = size % 4;
	if (val != 0) size += (4 - val);

	const u_int rqsz = size + sizeof(header_t);  // rqsz is short for required size
	bool only_one_free_block = (end == end->next);

	// iter point to the block that was last allocated
	if (!only_one_free_block) {
		iter = iter->next;
		void* ori_addr = iter;
		while (iter->next->size < rqsz) {
			iter = iter->next;

			// means there's not a big enough block
			if (iter == ori_addr){
				EXECUTE_HOOK(hook_alloc_failed, &size);
				enable_task_switch();
				return NULL;
			}
		}
	}
	block_t* new_block = iter->next;   

/* find a free block to allocate */
	block_t* splitted = NULL;
	if (new_block->size - rqsz >= sizeof(block_t)) {	// block have enough space to be splitted into two	
		splitted = (block_t*)((u_int)new_block + rqsz);
 		splitted->size = new_block->size - rqsz;

		if (only_one_free_block) {
			splitted->next = splitted;
			iter = splitted;
		}
		else {
			iter->next = splitted;
			splitted->next = new_block->next;
		}
	}
	else {
		if (only_one_free_block) {    	// only one free block left and it's about to be allocated,  heap memory is used up
			iter = NULL;
			end = NULL;
		}
		else {
			iter->next = new_block->next;
		}
	}
	/* split block */

	if (new_block == end)
		end = splitted ? splitted : iter;

	((header_t*)new_block)->size = size;
	((header_t*)new_block)->magic = 0x6D6Du;
/* write info to header_t */

	info.remain_size -= rqsz;
	info.malloc_count += 1;
	if (info.remain_size < min_left)
		min_left = info.remain_size;

	enable_task_switch();
	return (void*)((u_int)new_block + sizeof(header_t));
}


void* os_calloc(int nitems, int item_size){
	void *addr = os_malloc(nitems * item_size);
	if (addr == NULL)
		return NULL;
	memset(addr, 0, nitems*item_size);
	return addr;
}


void os_free(void *addr){
	if (addr == NULL)
		return;
	header_t* header = (header_t*)((u_int)addr - sizeof(header_t));
	if (header->magic != 0x6D6Du){
		// reason if failed to free:
		//	1. the address didn't come from malloc
		// 	2. heap overflow and change the magic number accidently
		EXECUTE_HOOK(hook_free_failed, addr);
		return;
	}

	disable_task_switch();
	block_t* rls = (block_t*)header;
	const u_int rls_size = header->size + sizeof(header_t);

	info.free_count += 1;
	info.remain_size += rls_size;

	// if whole memory used up
	if (end == NULL) {
		rls->next = rls;
		rls->size = rls_size;
		iter = end = rls;
		enable_task_switch();
		return;
	}

	block_t* left = end;
	if (rls < left) {
		while (left->next < rls) 	
			left = left->next;
	}
	block_t* right = left->next;
/* find prev and next node */

	left->next = rls;
	rls->next = right;
	rls->size = rls_size;
	block_t* merge_block = rls;

	if ((u_int)left + left->size == (u_int)rls) {
		merge_block = left;
		merge_block->size += rls_size;
		merge_block->next = right;

	/* left merge */
	}

	if ((u_int)rls + rls_size == (u_int)right) {
		merge_block->size += right->size;
		merge_block->next = right->next;

		if (iter == right)
			iter = merge_block;
		if (end == right)
			end = merge_block;
	/* right merge */
	}
	if (end < rls)
		end = merge_block;
	enable_task_switch();
}


splist *wait_for_free = NULL;

/*
@ brief: Not immediately free, instead add the block to a linked list, and actually free in idle task
*/
void queue_free(void *addr){
	enter_critical();
	splist *_addr = addr;
	_addr->next = wait_for_free;
	wait_for_free = _addr;
	exit_critical();
}


bool is_heap_addr(void *addr){
	u_int _addr = (u_int)addr;
	u_int _heap = (u_int)heap;
	if (_addr < _heap || _addr >= _heap + CFG_HEAP_SIZE)
		return false;
	return true;
}


int get_heap_remain_size(void){
	return info.remain_size;
}


heap_status_t* heap_status(void){
	info.peak_heap_usage = CFG_HEAP_SIZE - min_left;
	
	info.max_free_block_size = end->size;
	info.left_block_num = 1;
	block_t *tmp_iter = end->next;
	while (tmp_iter != end){
		info.left_block_num += 1;
		
		if (tmp_iter->size > info.max_free_block_size)
			info.max_free_block_size = tmp_iter->size;

		tmp_iter = tmp_iter->next;
	}
	return &info;
}


#endif  // CFG_ALLOW_DYNAMIC_ALLOC
