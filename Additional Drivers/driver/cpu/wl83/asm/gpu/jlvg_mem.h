

#ifndef __JLVG_MEM_H__
#define __JLVG_MEM_H__

#include <stdint.h>

void *jlvg_mem_malloc(uint32_t size);
void *jlvg_mem_realloc(void *p, uint32_t size);
void jlvg_mem_free(void *p);

#endif