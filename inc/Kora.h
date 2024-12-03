#ifndef _UNVS_H
#define _UNVS_H

#include "KoraDef.h"
#include "task.h"
#include "ipc.h"
#include "alloc.h"
#include "assert.h"


void enter_critical(void);
void exit_critical(void);
void Kora_start(void);


#endif
