#pragma once

#include "result.h"

#include <stdint.h>

typedef struct Memory Memory;

typedef struct
{
    CpuResult (*ReadByte)(Memory* memory, uint32_t addr);
    CpuResult (*ReadHalfword)(Memory* memory, uint32_t addr);
    CpuResult (*ReadWord)(Memory* memory, uint32_t addr);

    CpuResult (*WriteByte)(Memory* memory, uint32_t addr, uint8_t byte);
    CpuResult (*WriteHalfword)(Memory* memory, uint32_t addr, uint16_t halfword);
    CpuResult (*WriteWord)(Memory* memory, uint32_t addr, uint32_t word);
} MemoryVtbl;

typedef struct MemoryTrait
{
    Memory* mem;
    MemoryVtbl* vtbl;
} MemoryTrait;

#ifdef __cplusplus
extern "C" {
#endif

static inline CpuResult ReadByte(MemoryTrait mem, uint32_t addr)
{
    return mem.vtbl->ReadByte(mem.mem, addr);
}

static CpuResult ReadHalfword(MemoryTrait mem, uint32_t addr)
{
    return mem.vtbl->ReadHalfword(mem.mem, addr);
}

static CpuResult ReadWord(MemoryTrait mem, uint32_t addr)
{
    return mem.vtbl->ReadWord(mem.mem, addr);
}

static inline CpuResult WriteByte(MemoryTrait mem, uint32_t addr, uint8_t byte)
{
    return mem.vtbl->WriteByte(mem.mem, addr, byte);
}

static inline CpuResult WriteHalfword(MemoryTrait mem, uint32_t addr, uint16_t halfword)
{
    return mem.vtbl->WriteHalfword(mem.mem, addr, halfword);
}

static inline CpuResult WriteWord(MemoryTrait mem, uint32_t addr, uint32_t word)
{
    return mem.vtbl->WriteWord(mem.mem, addr, word);
}

#ifdef __cplusplus
}
#endif
