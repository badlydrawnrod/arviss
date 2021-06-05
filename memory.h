#pragma once

#include "result.h"

#include <stdint.h>

typedef struct ArvissMemory ArvissMemory;

typedef enum MemoryCode
{
    mcOK,
    mcLOAD_ACCESS_FAULT,
    mcSTORE_ACCESS_FAULT
} MemoryCode;

typedef struct ArvissMemoryVtbl
{
    uint8_t (*ReadByte)(const ArvissMemory* memory, uint32_t addr, MemoryCode* mc);
    uint16_t (*ReadHalfword)(const ArvissMemory* memory, uint32_t addr, MemoryCode* mc);
    uint32_t (*ReadWord)(const ArvissMemory* memory, uint32_t addr, MemoryCode* mc);

    void (*WriteByte)(ArvissMemory* memory, uint32_t addr, uint8_t byte, MemoryCode* mc);
    void (*WriteHalfword)(ArvissMemory* memory, uint32_t addr, uint16_t halfword, MemoryCode* mc);
    void (*WriteWord)(ArvissMemory* memory, uint32_t addr, uint32_t word, MemoryCode* mc);
} ArvissMemoryVtbl;

typedef struct ArvissMemoryTrait
{
    ArvissMemory* mem;
    ArvissMemoryVtbl* vtbl;
} ArvissMemoryTrait;

#ifdef __cplusplus
extern "C" {
#endif

static inline uint8_t ArvissReadByte(const ArvissMemoryTrait mem, uint32_t addr, MemoryCode* mc)
{
    return mem.vtbl->ReadByte(mem.mem, addr, mc);
}

static inline uint16_t ArvissReadHalfword(const ArvissMemoryTrait mem, uint32_t addr, MemoryCode* mc)
{
    return mem.vtbl->ReadHalfword(mem.mem, addr, mc);
}

static inline uint32_t ArvissReadWord(const ArvissMemoryTrait mem, uint32_t addr, MemoryCode* mc)
{
    return mem.vtbl->ReadWord(mem.mem, addr, mc);
}

static inline void ArvissWriteByte(ArvissMemoryTrait mem, uint32_t addr, uint8_t byte, MemoryCode* mc)
{
    mem.vtbl->WriteByte(mem.mem, addr, byte, mc);
}

static inline void ArvissWriteHalfword(ArvissMemoryTrait mem, uint32_t addr, uint16_t halfword, MemoryCode* mc)
{
    mem.vtbl->WriteHalfword(mem.mem, addr, halfword, mc);
}

static inline void ArvissWriteWord(ArvissMemoryTrait mem, uint32_t addr, uint32_t word, MemoryCode* mc)
{
    mem.vtbl->WriteWord(mem.mem, addr, word, mc);
}

#ifdef __cplusplus
}
#endif
