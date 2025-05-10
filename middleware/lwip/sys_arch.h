#ifndef _SYS_ARCH_H
#define _SYS_ARCH_H


#include "Kora.h"
#include "lwip/err.h"

typedef sem_t sys_sem_t;
typedef mutex_t sys_mutex_t;
typedef msgq_t sys_mbox_t;
typedef task_handle sys_thread_t;
typedef void (*lwip_thread_fn)(void*); 

#define SYS_MBOX_NULL   NULL
#define SYS_SEM_NULL    NULL
#define SYS_MRTEX_NULL  NULL
#define SYS_MBOX_EMPTY  NULL
#define SYS_ARCH_TIMEOUT   0xFFFFFFFFul

u_int sys_now(void);
void sys_init(void);
sys_prot_t sys_arch_protect(void);
void sys_arch_unprotect(sys_prot_t pval);
err_t sys_sem_new(sys_sem_t *sem, u_char count);
void sys_sem_free(sys_sem_t *sem);
int sys_sem_valid(sys_sem_t *sem);
void sys_sem_set_invalid(sys_sem_t *sem);
u_int sys_arch_sem_wait(sys_sem_t *sem, u_int timeout);
void sys_sem_signal(sys_sem_t *sem);
err_t sys_mutex_new(sys_mutex_t *mutex);
void sys_mutex_free(sys_mutex_t *mutex);
void sys_mutex_set_invalid(sys_mutex_t *mutex);
void sys_mutex_lock(sys_mutex_t *mutex);
void sys_mutex_unlock(sys_mutex_t *mutex);
err_t sys_mbox_new(sys_mbox_t *mbox, int size);
void sys_mbox_free(sys_mbox_t *mbox);
int sys_mbox_valid(sys_mbox_t *mbox);
void sys_mbox_set_invalid(sys_mbox_t *mbox);
void sys_mbox_post(sys_mbox_t *q, void *msg);
err_t sys_mbox_trypost(sys_mbox_t *q, void *msg);
err_t sys_mbox_trypost_fromisr(sys_mbox_t *q, void *msg);
u_int sys_arch_mbox_fetch(sys_mbox_t *q, void **msg, u_int timeout);
u_int sys_arch_mbox_tryfetch(sys_mbox_t *q, void **msg);
sys_thread_t sys_thread_new(const char *name, lwip_thread_fn function,void *arg, int stacksize, int prio);


#define sys_mbox_tryfetch(mbox, msg) sys_arch_mbox_tryfetch(mbox, msg)
#define sys_mbox_valid_val(mbox)     sys_mbox_valid(&(mbox))

#define SYS_ARCH_PROTECT(level)       sys_arch_protect()
#define SYS_ARCH_UNPROTECT(level)     sys_arch_unprotect(level)
#define SYS_ARCH_DECL_PROTECT(level)  sys_prot_t level

#ifndef SYS_ARCH_INC
#define SYS_ARCH_INC(var, val) do { \
                                SYS_ARCH_DECL_PROTECT(old_level); \
                                SYS_ARCH_PROTECT(old_level); \
                                var += val; \
                                SYS_ARCH_UNPROTECT(old_level); \
                              } while(0)
#endif /* SYS_ARCH_INC */

#ifndef SYS_ARCH_DEC
#define SYS_ARCH_DEC(var, val) do { \
                                SYS_ARCH_DECL_PROTECT(old_level); \
                                SYS_ARCH_PROTECT(old_level); \
                                var -= val; \
                                SYS_ARCH_UNPROTECT(old_level); \
                              } while(0)
#endif /* SYS_ARCH_DEC */

#ifndef SYS_ARCH_GET
#define SYS_ARCH_GET(var, ret) do { \
                                SYS_ARCH_DECL_PROTECT(old_level); \
                                SYS_ARCH_PROTECT(old_level); \
                                ret = var; \
                                SYS_ARCH_UNPROTECT(old_level); \
                              } while(0)
#endif /* SYS_ARCH_GET */

#ifndef SYS_ARCH_SET
#define SYS_ARCH_SET(var, val) do { \
                                SYS_ARCH_DECL_PROTECT(old_level); \
                                SYS_ARCH_PROTECT(old_level); \
                                var = val; \
                                SYS_ARCH_UNPROTECT(old_level); \
                              } while(0)
#endif /* SYS_ARCH_SET */

#ifndef SYS_ARCH_LOCKED
#define SYS_ARCH_LOCKED(code) do { \
                                SYS_ARCH_DECL_PROTECT(old_level); \
                                SYS_ARCH_PROTECT(old_level); \
                                code; \
                                SYS_ARCH_UNPROTECT(old_level); \
                              } while(0)
#endif /* SYS_ARCH_LOCKED */



#endif
