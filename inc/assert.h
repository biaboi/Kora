#ifndef _ASSERT_H
#define _ASSERT_H

#include "KoraConfig.h"

#if CFG_KORA_ASSERT
	#define os_assert(value) ((value) ? (void)0 : os_assert_failed((char *)__FILE__, __LINE__))
	void os_assert_failed(char *, int);
#else
	#define os_assert(value) ((void)0)
#endif

#endif
