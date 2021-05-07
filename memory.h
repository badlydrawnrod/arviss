#pragma once

#include "result.h"

#include <stdint.h>

typedef struct ArvissMemory ArvissMemory;

typedef struct
{
    ArvissResult (*ReadByte)(ArvissMemory* memory, uint32_t addr);
    ArvissResult (*ReadHalfword)(ArvissMemory* memory, uint32_t addr);
    ArvissResult (*ReadWord)(ArvissMemory* memory, uint32_t addr);

    ArvissResult (*WriteByte)(ArvissMemory* memory, uint32_t addr, uint8_t byte);
    ArvissResult (*WriteHalfword)(ArvissMemory* memory, uint32_t addr, uint16_t halfword);
    ArvissResult (*WriteWord)(ArvissMemory* memory, uint32_t addr, uint32_t word);
} ArvissMemoryVtbl;

typedef struct MemoryTrait
{
    ArvissMemory* mem;
    ArvissMemoryVtbl* vtbl;
} ArvissMemoryTrait;

#ifdef __cplusplus
extern "C" {
#endif

static inline ArvissResult ArvissReadByte(ArvissMemoryTrait mem, uint32_t addr)
{
    return mem.vtbl->ReadByte(mem.mem, addr);
}

static ArvissResult ArvissReadHalfword(ArvissMemoryTrait mem, uint32_t addr)
{
    return mem.vtbl->ReadHalfword(mem.mem, addr);
}

static ArvissResult ArvissReadWord(ArvissMemoryTrait mem, uint32_t addr)
{
    return mem.vtbl->ReadWord(mem.mem, addr);
}

static inline ArvissResult ArvissWriteByte(ArvissMemoryTrait mem, uint32_t addr, uint8_t byte)
{
    return mem.vtbl->WriteByte(mem.mem, addr, byte);
}

static inline ArvissResult ArvissWriteHalfword(ArvissMemoryTrait mem, uint32_t addr, uint16_t halfword)
{
    return mem.vtbl->WriteHalfword(mem.mem, addr, halfword);
}

static inline ArvissResult ArvissWriteWord(ArvissMemoryTrait mem, uint32_t addr, uint32_t word)
{
    return mem.vtbl->WriteWord(mem.mem, addr, word);
}

#ifdef __cplusplus
}
#endif
