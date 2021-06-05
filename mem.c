#include "mem.h"

#include <stdint.h>
#include <stdio.h>

#define TTY_STATUS IOBASE
#define TTY_DATA (TTY_STATUS + 1)
#define TTY_IS_WRITABLE 0x01

static const uint32_t membase = MEMBASE;
static const uint32_t memsize = MEMSIZE;
static const uint32_t rambase = RAMBASE;
static const uint32_t ramsize = RAMSIZE;

static uint8_t ReadByte(const ArvissMemory* memory, uint32_t addr, MemoryCode* mc);
static uint16_t ReadHalfword(const ArvissMemory* memory, uint32_t addr, MemoryCode* mc);
static uint32_t ReadWord(const ArvissMemory* memory, uint32_t addr, MemoryCode* mc);
static void WriteByte(ArvissMemory* memory, uint32_t addr, uint8_t byte, MemoryCode* mc);
static void WriteHalfword(ArvissMemory* memory, uint32_t addr, uint16_t halfword, MemoryCode* mc);
static void WriteWord(ArvissMemory* memory, uint32_t addr, uint32_t word, MemoryCode* mc);

static ArvissMemoryVtbl vtbl = {.ReadByte = ReadByte,
                                .ReadHalfword = ReadHalfword,
                                .ReadWord = ReadWord,
                                .WriteByte = WriteByte,
                                .WriteHalfword = WriteHalfword,
                                .WriteWord = WriteWord};

ArvissMemoryTrait MemInit(ArvissMemory* memory)
{
    return (ArvissMemoryTrait){.mem = memory, .vtbl = &vtbl};
}

static uint8_t ReadByte(const ArvissMemory* memory, uint32_t addr, MemoryCode* mc)
{
    if (addr >= membase && addr < membase + memsize)
    {
        return memory->mem[addr - membase];
    }

    if (addr == TTY_STATUS)
    {
        return 0xff; // TODO: return a real status.
    }

    *mc = mcLOAD_ACCESS_FAULT, addr;
    return -1;
}

static uint16_t ReadHalfword(const ArvissMemory* memory, uint32_t addr, MemoryCode* mc)
{
    if (addr >= membase && addr < membase + memsize - 1)
    {
        return memory->mem[addr - membase] | (memory->mem[addr + 1 - membase] << 8);
    }

    *mc = mcLOAD_ACCESS_FAULT, addr;
    return -1;
}

static uint32_t ReadWord(const ArvissMemory* memory, uint32_t addr, MemoryCode* mc)
{
    if (addr >= membase && addr < membase + memsize - 3)
    {
        const uint8_t* base = &memory->mem[addr - membase];
        return base[0] | (base[1] << 8) | (base[2] << 16) | (base[3] << 24);
    }

    *mc = mcLOAD_ACCESS_FAULT;
    return -1;
}

static void WriteByte(ArvissMemory* memory, uint32_t addr, uint8_t byte, MemoryCode* mc)
{
    if (addr >= rambase && addr < rambase + ramsize)
    {
        memory->mem[addr - membase] = byte;
        return;
    }

    if (addr == TTY_DATA) // TODO: only write when the status says that we can.
    {
        //        putchar(byte);
        return;
    }

    *mc = mcSTORE_ACCESS_FAULT;
}

static void WriteHalfword(ArvissMemory* memory, uint32_t addr, uint16_t halfword, MemoryCode* mc)
{
    if (addr >= rambase && addr < rambase + ramsize - 1)
    {
        memory->mem[addr - membase] = halfword & 0xff;
        memory->mem[addr + 1 - membase] = (halfword >> 8) & 0xff;
        return;
    }

    *mc = mcSTORE_ACCESS_FAULT;
}

static void WriteWord(ArvissMemory* memory, uint32_t addr, uint32_t word, MemoryCode* mc)
{
    if (addr >= rambase && addr < rambase + ramsize - 2)
    {
        memory->mem[addr - membase] = word & 0xff;
        memory->mem[addr + 1 - membase] = (word >> 8) & 0xff;
        memory->mem[addr + 2 - membase] = (word >> 16) & 0xff;
        memory->mem[addr + 3 - membase] = (word >> 24) & 0xff;
        return;
    }

    *mc = mcSTORE_ACCESS_FAULT;
}
