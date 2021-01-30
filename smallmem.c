#include "smallmem.h"

#include <stdint.h>

static const uint32_t rambase = RAMBASE;
static const uint32_t ramsize = RAMSIZE;

static CpuResult smallmem_ReadByte(Memory* memory, uint32_t addr);
static CpuResult smallmem_ReadHalfword(Memory* memory, uint32_t addr);
static CpuResult smallmem_ReadWord(Memory* memory, uint32_t addr);
static CpuResult smallmem_WriteByte(Memory* memory, uint32_t addr, uint8_t byte);
static CpuResult smallmem_WriteHalfword(Memory* memory, uint32_t addr, uint16_t halfword);
static CpuResult smallmem_WriteWord(Memory* memory, uint32_t addr, uint32_t word);

static MemoryVtbl vtbl = {smallmem_ReadByte,  smallmem_ReadHalfword,  smallmem_ReadWord,
                          smallmem_WriteByte, smallmem_WriteHalfword, smallmem_WriteWord};

MemoryTrait smallmem_Init(Memory* memory)
{
    return (MemoryTrait){.mem = memory, .vtbl = &vtbl};
}

static CpuResult smallmem_ReadByte(Memory* memory, uint32_t addr)
{
    if (addr >= rambase && addr < rambase + ramsize)
    {
        return MakeByte(memory->ram[addr - rambase]);
    }
    return MakeTrap(trLOAD_ACCESS_FAULT, addr);
}

static CpuResult smallmem_ReadHalfword(Memory* memory, uint32_t addr)
{
    if (addr >= rambase && addr < rambase + ramsize - 1)
    {
        return MakeHalfword(memory->ram[addr - rambase] | (memory->ram[addr + 1 - rambase] << 8));
    }
    return MakeTrap(trLOAD_ACCESS_FAULT, addr);
}

static CpuResult smallmem_ReadWord(Memory* memory, uint32_t addr)
{
    if (addr >= rambase && addr < rambase + ramsize - 3)
    {
        return MakeWord(memory->ram[addr - rambase] | (memory->ram[addr + 1 - rambase] << 8)
                        | (memory->ram[addr + 2 - rambase] << 16) | (memory->ram[addr + 3 - rambase] << 24));
    }
    return MakeTrap(trLOAD_ACCESS_FAULT, addr);
}

static CpuResult smallmem_WriteByte(Memory* memory, uint32_t addr, uint8_t byte)
{
    if (addr >= rambase && addr < rambase + ramsize)
    {
        memory->ram[addr - rambase] = byte;
        return MakeOk();
    }
    return MakeTrap(trSTORE_ACCESS_FAULT, addr);
}

static CpuResult smallmem_WriteHalfword(Memory* memory, uint32_t addr, uint16_t halfword)
{
    if (addr >= rambase && addr < rambase + ramsize - 1)
    {
        memory->ram[addr - rambase] = halfword & 0xff;
        memory->ram[addr + 1 - rambase] = (halfword >> 8) & 0xff;
        return MakeOk();
    }
    return MakeTrap(trSTORE_ACCESS_FAULT, addr);
}

static CpuResult smallmem_WriteWord(Memory* memory, uint32_t addr, uint32_t word)
{
    if (addr >= rambase && addr < rambase + ramsize - 3)
    {
        memory->ram[addr - rambase] = word & 0xff;
        memory->ram[addr + 1 - rambase] = (word >> 8) & 0xff;
        memory->ram[addr + 2 - rambase] = (word >> 16) & 0xff;
        memory->ram[addr + 3 - rambase] = (word >> 24) & 0xff;
        return MakeOk();
    }
    return MakeTrap(trSTORE_ACCESS_FAULT, addr);
}
