#ifndef _UNVS_H
#define _UNVS_H

#include "KoraDef.h"
#include "task.h"
#include "ipc.h"
#include "alloc.h"
#include "list.h"
#include "assert.h"


void enter_critical(void);
void exit_critical(void);
void Kora_start(void);

/*
	hardware config: 
		SVC, Systick, Pendsv enable and priority set
		Systick frequence set
*/


#endif
