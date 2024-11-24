#include "Kora.h"
#include "ff.h"
/*------------------------------------------------------------------------*/
/*  fatfs(ver.0.15) multi-task interface based on Kora                    */
/*------------------------------------------------------------------------*/


#if FF_USE_LFN == 3/* Use dynamic memory allocation */

 #if CFG_ALLOW_DYNAMIC_ALLOC == 0
  #error "Kora must enable dynamic alloc"
 #endif

	void* ff_memalloc (UINT msize){
		return malloc((size_t)msize);
	}


	void ff_memfree (void* mblock){
		queue_free(mblock);
	}

#endif


#if FF_FS_REENTRANT 
	
 // #if CFG_ALLOW_DYNAMIC_ALLOC == 1
// 	static mutex_t Mutex[FF_VOLUMES + 1];

// 	int ff_mutex_create(int vol){
// 		Mutex[vol] = mutex_create();
// 		return Mutex[vol] != NULL;
// 	}

// 	void ff_mutex_delete(int vol){
// 		mutex_delete(Mutex[vol]);
// 	}

// 	int ff_mutex_take(int vol){
// 		mutex_lock(Mutex[vol]);
// 		return 1;
// 	}

// 	void ff_mutex_give(int vol){
// 		mutex_unlock(Mutex[vol]);
// 	}

 // #else
	static mutex Mutex[FF_VOLUMES + 1];

	int ff_mutex_create(int vol){
		mutex_init(Mutex+vol);
		return 1;
	}

	void ff_mutex_delete(int vol){
		return ;
	}

	int ff_mutex_take(int vol){
		mutex_lock(Mutex+vol);
		return 1;
	}

	void ff_mutex_give(int vol){
		mutex_unlock(Mutex+vol);
	}
 // #endif


#endif	/* FF_FS_REENTRANT */
