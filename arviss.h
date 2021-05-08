#pragma once

#include "memory.h"
#include "result.h"

#include <stdint.h>

typedef struct ArvissCpu
{
    uint32_t pc;       // The program counter.
    uint32_t xreg[32]; // Regular registers, x0-x31.
    uint32_t mepc;     // The machine exception program counter.
    uint32_t mcause;   // The machine cause register.
    uint32_t mtval;    // The machine trap value register.
    float freg[32];    // Floating point registers, f0-f31.
    uint32_t fcsr;     // Floating point control and status register.

    ArvissMemoryTrait memory;
} ArvissCpu;

#ifdef __cplusplus
extern "C" {
#endif

void ArvissReset(ArvissCpu* cpu, uint32_t sp);
ArvissResult ArvissExecute(ArvissCpu* cpu, uint32_t instruction);
ArvissResult ArvissFetch(ArvissCpu* cpu);
ArvissResult ArvissHandleTrap(ArvissCpu* cpu, ArvissTrap trap);
ArvissResult ArvissRun(ArvissCpu* cpu, int count);

#ifdef __cplusplus
}
#endif
