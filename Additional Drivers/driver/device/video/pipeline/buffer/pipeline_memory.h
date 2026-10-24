#ifndef __MEMORY_H
#define __MEMORY_H

#include "asm/cpu.h"

typedef enum {
    DDR_MEMORY = 0,    ///< Surface
    RAM_MEMORY,   ///< Share Memory fd
    RAM_CACHE_MEMORY,
    UNKNOW_MEMORY,
} memory_type_t;

typedef struct {
    void *mem_addr;
    size_t size;
    size_t offset;
    memory_type_t type;
} memory;

int memory_alloc_static(memory_type_t type, memory *mem, int size);

void  memory_free(memory *mem);

uint8_t *get_mem_addr(memory *mem);

u32 get_mem_size(memory *mem);

u8 *video_ram_malloc(u32 size);
u8 *video_ram_cache_malloc(u32 size, u8 align);
void video_ram_cache_free(u8 *buf);
void video_ram_free(u8 *buf);

#endif
