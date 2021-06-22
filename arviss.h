#pragma once

#include "memory.h"
#include "result.h"

#include <stdint.h>

typedef struct ArvissCpu ArvissCpu;

#ifdef __cplusplus
extern "C" {
#endif

typedef struct ArvissDesc
{
    ArvissMemoryTrait memory;
} ArvissDesc;

ArvissCpu* ArvissCreate(const ArvissDesc* desc);
void ArvissDispose(ArvissCpu* cpu);

void ArvissReset(ArvissCpu* cpu, uint32_t sp);
ArvissResult ArvissExecute(ArvissCpu* cpu, uint32_t instruction);
ArvissResult ArvissRun(ArvissCpu* cpu, int count);
uint32_t ArvissReadXReg(ArvissCpu* cpu, int reg);
void ArvissWriteXReg(ArvissCpu* cpu, int reg, uint32_t value);
ArvissMemoryTrait ArvissGetMemory(ArvissCpu* cpu);
void ArvissMret(ArvissCpu* cpu);

#ifdef __cplusplus
}
#endif
