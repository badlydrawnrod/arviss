#include "smallmem.h"

#include <stdint.h>

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

ArvissMemoryTrait SmallMemInit(ArvissMemory* memory)
{
    return (ArvissMemoryTrait){.mem = memory, .vtbl = &vtbl};
}

static ArvissResult ReadByte(const ArvissMemory* memory, uint32_t addr)
{
    if (addr >= rambase && addr < rambase + ramsize)
    {
        return ArvissMakeByte(memory->ram[addr - rambase]);
    }
    return ArvissMakeTrap(trLOAD_ACCESS_FAULT, addr);
}

static ArvissResult ReadHalfword(const ArvissMemory* memory, uint32_t addr)
{
    if (addr >= rambase && addr < rambase + ramsize - 1)
    {
        return ArvissMakeHalfword(memory->ram[addr - rambase] | (memory->ram[addr + 1 - rambase] << 8));
    }
    return ArvissMakeTrap(trLOAD_ACCESS_FAULT, addr);
}

static ArvissResult ReadWord(const ArvissMemory* memory, uint32_t addr)
{
    if (addr >= rambase && addr < rambase + ramsize - 3)
    {
        return ArvissMakeWord(memory->ram[addr - rambase] | (memory->ram[addr + 1 - rambase] << 8)
                              | (memory->ram[addr + 2 - rambase] << 16) | (memory->ram[addr + 3 - rambase] << 24));
    }
    return ArvissMakeTrap(trLOAD_ACCESS_FAULT, addr);
}

static ArvissResult WriteByte(ArvissMemory* memory, uint32_t addr, uint8_t byte)
{
    if (addr >= rambase && addr < rambase + ramsize)
    {
        memory->ram[addr - rambase] = byte;
        return ArvissMakeOk();
    }
    return ArvissMakeTrap(trSTORE_ACCESS_FAULT, addr);
}

static ArvissResult WriteHalfword(ArvissMemory* memory, uint32_t addr, uint16_t halfword)
{
    if (addr >= rambase && addr < rambase + ramsize - 1)
    {
        memory->ram[addr - rambase] = halfword & 0xff;
        memory->ram[addr + 1 - rambase] = (halfword >> 8) & 0xff;
        return ArvissMakeOk();
    }
    return ArvissMakeTrap(trSTORE_ACCESS_FAULT, addr);
}

static ArvissResult WriteWord(ArvissMemory* memory, uint32_t addr, uint32_t word)
{
    if (addr >= rambase && addr < rambase + ramsize - 3)
    {
        memory->ram[addr - rambase] = word & 0xff;
        memory->ram[addr + 1 - rambase] = (word >> 8) & 0xff;
        memory->ram[addr + 2 - rambase] = (word >> 16) & 0xff;
        memory->ram[addr + 3 - rambase] = (word >> 24) & 0xff;
        return ArvissMakeOk();
    }
    return ArvissMakeTrap(trSTORE_ACCESS_FAULT, addr);
}
