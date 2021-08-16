#pragma once

#define ROM_START 0
#define ROMSIZE 0x4000
#define RAMBASE (ROM_START + ROMSIZE)
#define RAMSIZE 0x4000

#define MEMBASE ROM_START
#define MEMSIZE (ROMSIZE + RAMSIZE)

#define IOBASE (RAMBASE + RAMSIZE)

typedef struct
{
    uint8_t mem[MEMSIZE];
} Memory;
