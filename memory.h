#pragma once

#include "result.h"

#include <stdint.h>

typedef enum MemoryCode
{
    mcOK,
    mcLOAD_ACCESS_FAULT,
    mcSTORE_ACCESS_FAULT
} MemoryCode;

typedef struct ArvissMemory ArvissMemory;

#ifdef __cplusplus
extern "C" {
#endif

uint8_t ArvissReadByte(const ArvissMemory* memory, uint32_t addr, MemoryCode* mc);
uint16_t ArvissReadHalfword(const ArvissMemory* memory, uint32_t addr, MemoryCode* mc);
uint32_t ArvissReadWord(const ArvissMemory* memory, uint32_t addr, MemoryCode* mc);
void ArvissWriteByte(ArvissMemory* memory, uint32_t addr, uint8_t byte, MemoryCode* mc);
void ArvissWriteHalfword(ArvissMemory* memory, uint32_t addr, uint16_t halfword, MemoryCode* mc);
void ArvissWriteWord(ArvissMemory* memory, uint32_t addr, uint32_t word, MemoryCode* mc);

#ifdef __cplusplus
}
#endif
