#pragma once

#include "memory.h"

#define RAMBASE 0
#define RAMSIZE 0x8000

struct Memory
{
    uint8_t ram[RAMSIZE];
};

#ifdef __cplusplus
extern "C" {
#endif

MemoryTrait smallmem_Init(Memory* memory);

#ifdef __cplusplus
}
#endif
