#include "debug.h"

#include <lwip/opt.h>
#include <lwip/arch.h>

#include "tcpip.h"
#include "lwip/init.h"
#include "lwip/netif.h"
#include "lwip/sio.h"
#include "ethernetif.h"
#if !NO_SYS
 #include "sys_arch.h"
#endif
#include <lwip/stats.h>
#include <lwip/debug.h>
#include <lwip/sys.h>

#include <string.h>
#include "kora.h"

// lwip version: 2.1

u_int sys_now(void) {
	return os_get_tick();
}

void sys_init(void) {

}

sys_prot_t sys_arch_protect(void){
	enter_critical();
	return 0;
}

void sys_arch_unprotect(sys_prot_t pval) {
	exit_critical();
}

err_t sys_sem_new(sys_sem_t *sem, u_char count) {
	*sem = sem_create(count, count);
	if (*sem == NULL)
		return ERR_MEM;
	return ERR_OK;
}

void sys_sem_free(sys_sem_t *sem){
	sem_delete(*sem);
}

int sys_sem_valid(sys_sem_t *sem) {
    return (*sem != SYS_SEM_NULL);
}

void sys_sem_set_invalid(sys_sem_t *sem) {
	*sem = NULL;
}

u_int sys_arch_sem_wait(sys_sem_t *sem, u_int timeout){
	int ret = sem_wait(*sem, timeout);
	if (ret == RET_FAILED)
		return SYS_ARCH_TIMEOUT;
	else
		return 1;
}

void sys_sem_signal(sys_sem_t *sem) {
	sem_signal(*sem);
}

err_t sys_mutex_new(sys_mutex_t *mutex){
	*mutex = mutex_create();
	if (*mutex == NULL)
		return ERR_MEM;
	return ERR_OK;
}

void sys_mutex_free(sys_mutex_t *mutex) {
	mutex_delete(*mutex);	
}

void sys_mutex_set_invalid(sys_mutex_t *mutex) {
	*mutex = NULL;
}

void sys_mutex_lock(sys_mutex_t *mutex) {
	mutex_lock(*mutex);
}

void sys_mutex_unlock(sys_mutex_t *mutex) {
    mutex_unlock(*mutex);
}

err_t sys_mbox_new(sys_mbox_t *mbox, int size) {
	if (size == 0)
		size = 8;
	*mbox = msgq_create(size, sizeof(void*));
	if (*mbox == NULL)
		return ERR_MEM;
	return ERR_OK;
}

void sys_mbox_free(sys_mbox_t *mbox) {
	// int left = MSGQUE_LEN(mbox);
	msgq_delete(*mbox);
}

int sys_mbox_valid(sys_mbox_t *mbox) {
	return *mbox != NULL;
}

void sys_mbox_set_invalid(sys_mbox_t *mbox) {
    *mbox = NULL;
}

void sys_mbox_post(sys_mbox_t *q, void *msg) {
	msgq_push(*q, &msg, FOREVER);
}

err_t sys_mbox_trypost(sys_mbox_t *q, void *msg) {
	int ret = msgq_push(*q, &msg, 0);
	if (ret != RET_SUCCESS)
		return ERR_MEM;

	return ERR_OK;
}

err_t sys_mbox_trypost_fromisr(sys_mbox_t *q, void *msg) {
	if (MSGQUE_FULL(*q)){
		return ERR_MEM;
	}

	msgq_overwrite_isr(*q, &msg);
	return ERR_OK;
}

u_int sys_arch_mbox_fetch(sys_mbox_t *q, void **msg, u_int timeout) {
	u_int stamp = os_get_tick();
	int ret = msgq_front(*q, msg, timeout);

	if (ret == RET_FAILED){
		msg = NULL;
		return SYS_ARCH_TIMEOUT;
	}
	msgq_pop(*q);

	return os_get_tick() - stamp;
}

u_int sys_arch_mbox_tryfetch(sys_mbox_t *q, void **msg) {
	if (msgq_front(*q, msg, 0) == RET_FAILED)
		return SYS_ARCH_TIMEOUT;
	
	msgq_pop(*q);
	return 1;
}

sys_thread_t sys_thread_new(const char *name, lwip_thread_fn function,
        void *arg, int stacksize, int prio) {
	return task_create(function, name, arg, prio, stacksize);
}
