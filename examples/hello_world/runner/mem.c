#include "mem.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define TTY_STATUS IOBASE
#define TTY_DATA (TTY_STATUS + 1)
#define TTY_IS_WRITABLE 0x01

static const uint32_t membase = MEMBASE;
static const uint32_t memsize = MEMSIZE;
static const uint32_t rambase = RAMBASE;
static const uint32_t ramsize = RAMSIZE;

ArvissMemory* ArvissCreateMem(void)
{
    return calloc(1, sizeof(ArvissMemory));
}

void ArvissFreeMem(ArvissMemory* memory)
{
    free(memory);
}

uint8_t ArvissReadByte(const ArvissMemory* memory, uint32_t addr, MemoryCode* mc)
{
    if (addr >= membase && addr < membase + memsize)
    {
        return memory->mem[addr - membase];
    }

    if (addr == TTY_STATUS)
    {
        return 0xff; // TODO: return a real status.
    }

    *mc = mcLOAD_ACCESS_FAULT;
    return 0;
}

uint16_t ArvissReadHalfword(const ArvissMemory* memory, uint32_t addr, MemoryCode* mc)
{
    if (addr >= membase && addr < membase + memsize - 1)
    {
        // TODO: implement for big-endian ISAs.
        const uint16_t* base = (uint16_t*)&memory->mem[addr - membase];
        return *base;
    }

    *mc = mcLOAD_ACCESS_FAULT;
    return 0;
}

uint32_t ArvissReadWord(const ArvissMemory* memory, uint32_t addr, MemoryCode* mc)
{
    if (addr >= membase && addr < membase + memsize - 3)
    {
        // TODO: implement for big-endian ISAs.
        const uint32_t* base = (uint32_t*)&memory->mem[addr - membase];
        return *base;
    }

    *mc = mcLOAD_ACCESS_FAULT;
    return 0;
}

void ArvissWriteByte(ArvissMemory* memory, uint32_t addr, uint8_t byte, MemoryCode* mc)
{
    if (addr >= rambase && addr < rambase + ramsize)
    {
        memory->mem[addr - membase] = byte;
        return;
    }

    if (addr == TTY_DATA) // TODO: only write when the status says that we can.
    {
        putchar(byte);
        return;
    }

    *mc = mcSTORE_ACCESS_FAULT;
}

void ArvissWriteHalfword(ArvissMemory* memory, uint32_t addr, uint16_t halfword, MemoryCode* mc)
{
    if (addr >= rambase && addr < rambase + ramsize - 1)
    {
        // TODO: implement for big-endian ISAs.
        uint16_t* base = (uint16_t*)&memory->mem[addr - membase];
        *base = halfword;
        return;
    }

    *mc = mcSTORE_ACCESS_FAULT;
}

void ArvissWriteWord(ArvissMemory* memory, uint32_t addr, uint32_t word, MemoryCode* mc)
{
    if (addr >= rambase && addr < rambase + ramsize - 2)
    {
        // TODO: implement for big-endian ISAs.
        uint32_t* base = (uint32_t*)&memory->mem[addr - membase];
        *base = word;
        return;
    }

    *mc = mcSTORE_ACCESS_FAULT;
}
