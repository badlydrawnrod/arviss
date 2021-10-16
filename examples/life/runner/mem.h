#pragma once

#define ROM_START 0
#define ROMSIZE 0x100
#define RAMBASE (ROM_START + ROMSIZE)
#define RAMSIZE 0x100

#define MEMBASE ROM_START
#define MEMSIZE (ROMSIZE + RAMSIZE)

typedef struct
{
    uint8_t mem[MEMSIZE];
} Memory;
