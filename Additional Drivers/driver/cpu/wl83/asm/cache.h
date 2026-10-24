#ifndef __ASM_CACHE_H__
#define __ASM_CACHE_H__

#include "generic/typedef.h"

#include "asm/icache.h"
#include "asm/dcache.h"

#define WAIT_ICACHE_IDLE	do {IcuWaitIdle();} while(0);
#define WAIT_DCACHE_IDLE	do {DcuWaitIdle();} while(0);

#endif
