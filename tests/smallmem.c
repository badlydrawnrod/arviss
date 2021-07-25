#include "smallmem.h"

#include <stdint.h>

static const uint32_t rambase = RAMBASE;
static const uint32_t ramsize = RAMSIZE;

uint8_t ArvissReadByte(const ArvissMemory* memory, uint32_t addr, MemoryCode* mc)
{
    if (addr >= rambase && addr < rambase + ramsize)
    {
        return memory->ram[addr - rambase];
    }

    *mc = mcLOAD_ACCESS_FAULT;
    return 0;
}

uint16_t ArvissReadHalfword(const ArvissMemory* memory, uint32_t addr, MemoryCode* mc)
{
    if (addr >= rambase && addr < rambase + ramsize - 1)
    {
        return memory->ram[addr - rambase] | (memory->ram[addr + 1 - rambase] << 8);
    }

    *mc = mcLOAD_ACCESS_FAULT;
    return 0;
}

uint32_t ArvissReadWord(const ArvissMemory* memory, uint32_t addr, MemoryCode* mc)
{
    if (addr >= rambase && addr < rambase + ramsize - 3)
    {
        return memory->ram[addr - rambase] | (memory->ram[addr + 1 - rambase] << 8) | (memory->ram[addr + 2 - rambase] << 16)
                | (memory->ram[addr + 3 - rambase] << 24);
    }

    *mc = mcLOAD_ACCESS_FAULT;
    return 0;
}

void ArvissWriteByte(ArvissMemory* memory, uint32_t addr, uint8_t byte, MemoryCode* mc)
{
    if (addr >= rambase && addr < rambase + ramsize)
    {
        memory->ram[addr - rambase] = byte;
        return;
    }

    *mc = mcSTORE_ACCESS_FAULT;
}

void ArvissWriteHalfword(ArvissMemory* memory, uint32_t addr, uint16_t halfword, MemoryCode* mc)
{
    if (addr >= rambase && addr < rambase + ramsize - 1)
    {
        memory->ram[addr - rambase] = halfword & 0xff;
        memory->ram[addr + 1 - rambase] = (halfword >> 8) & 0xff;
        return;
    }

    *mc = mcSTORE_ACCESS_FAULT;
}

void ArvissWriteWord(ArvissMemory* memory, uint32_t addr, uint32_t word, MemoryCode* mc)
{
    if (addr >= rambase && addr < rambase + ramsize - 3)
    {
        memory->ram[addr - rambase] = word & 0xff;
        memory->ram[addr + 1 - rambase] = (word >> 8) & 0xff;
        memory->ram[addr + 2 - rambase] = (word >> 16) & 0xff;
        memory->ram[addr + 3 - rambase] = (word >> 24) & 0xff;
        return;
    }

    *mc = mcSTORE_ACCESS_FAULT;
}
