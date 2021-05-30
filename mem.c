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

static ArvissResult ReadByte(const ArvissMemory* memory, uint32_t addr);
static ArvissResult ReadHalfword(const ArvissMemory* memory, uint32_t addr);
static ArvissResult ReadWord(const ArvissMemory* memory, uint32_t addr);
static ArvissResult WriteByte(ArvissMemory* memory, uint32_t addr, uint8_t byte);
static ArvissResult WriteHalfword(ArvissMemory* memory, uint32_t addr, uint16_t halfword);
static ArvissResult WriteWord(ArvissMemory* memory, uint32_t addr, uint32_t word);

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

static ArvissResult ReadByte(const ArvissMemory* memory, uint32_t addr)
{
    if (addr >= membase && addr < membase + memsize)
    {
        return ArvissMakeByte(memory->mem[addr - membase]);
    }

    if (addr == TTY_STATUS)
    {
        return ArvissMakeByte(0xff); // TODO: return a real status.
    }

    return ArvissMakeTrap(trLOAD_ACCESS_FAULT, addr);
}

static ArvissResult ReadHalfword(const ArvissMemory* memory, uint32_t addr)
{
    if (addr >= membase && addr < membase + memsize - 1)
    {
        return ArvissMakeHalfword(memory->mem[addr - membase] | (memory->mem[addr + 1 - membase] << 8));
    }

    return ArvissMakeTrap(trLOAD_ACCESS_FAULT, addr);
}

static ArvissResult ReadWord(const ArvissMemory* memory, uint32_t addr)
{
    if (addr >= membase && addr < membase + memsize - 3)
    {
        const uint8_t* base = &memory->mem[addr - membase];
        return ArvissMakeWord(base[0] | (base[1] << 8) | (base[2] << 16) | (base[3] << 24));
    }

    return ArvissMakeTrap(trLOAD_ACCESS_FAULT, addr);
}

static ArvissResult WriteByte(ArvissMemory* memory, uint32_t addr, uint8_t byte)
{
    if (addr >= rambase && addr < rambase + ramsize)
    {
        memory->mem[addr - membase] = byte;
        return ArvissMakeOk();
    }

    if (addr == TTY_DATA) // TODO: only write when the status says that we can.
    {
        putchar(byte);
        return ArvissMakeOk();
    }

    return ArvissMakeTrap(trSTORE_ACCESS_FAULT, addr);
}

static ArvissResult WriteHalfword(ArvissMemory* memory, uint32_t addr, uint16_t halfword)
{
    if (addr >= rambase && addr < rambase + ramsize - 1)
    {
        memory->mem[addr - membase] = halfword & 0xff;
        memory->mem[addr + 1 - membase] = (halfword >> 8) & 0xff;
        return ArvissMakeOk();
    }

    return ArvissMakeTrap(trSTORE_ACCESS_FAULT, addr);
}

static ArvissResult WriteWord(ArvissMemory* memory, uint32_t addr, uint32_t word)
{
    if (addr >= rambase && addr < rambase + ramsize - 2)
    {
        memory->mem[addr - membase] = word & 0xff;
        memory->mem[addr + 1 - membase] = (word >> 8) & 0xff;
        memory->mem[addr + 2 - membase] = (word >> 16) & 0xff;
        memory->mem[addr + 3 - membase] = (word >> 24) & 0xff;
        return ArvissMakeOk();
    }

    return ArvissMakeTrap(trSTORE_ACCESS_FAULT, addr);
}
