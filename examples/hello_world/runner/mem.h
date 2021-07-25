#pragma once

#include "memory.h"

#define ROM_START 0
#define ROMSIZE 0x4000
#define RAMBASE (ROM_START + ROMSIZE)
#define RAMSIZE 0x4000

#define MEMBASE ROM_START
#define MEMSIZE (ROMSIZE + RAMSIZE)

#define IOBASE (RAMBASE + RAMSIZE)

struct ArvissMemory
{
    uint8_t mem[MEMSIZE];
};
