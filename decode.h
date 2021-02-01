#pragma once

#include "memory.h"
#include "result.h"

#include <stdbool.h>
#include <stdint.h>

typedef struct CPU
{
    uint32_t pc;       // The program counter.
    uint32_t xreg[32]; // Regular registers, x0-x31.
    uint32_t mepc;     // The machine exception program counter.
    uint32_t mcause;   // The machine cause register.
    uint32_t mtval;    // The machine trap value register.
    float freg[32];    // Floating point registers, f0-f31.
    uint32_t fcsr;     // Floating point control and status register.

    MemoryTrait memory;

} CPU;

enum
{
    OP_LUI = 0b0110111,
    OP_AUIPC = 0b0010111,
    OP_JAL = 0b1101111,
    OP_JALR = 0b1100111,
    OP_BRANCH = 0b1100011,
    OP_LOAD = 0b0000011,
    OP_STORE = 0b0100011,
    OP_OPIMM = 0b0010011,
    OP_OP = 0b0110011,
    OP_MISCMEM = 0b0001111,
    OP_SYSTEM = 0b1110011,
    OP_LOADFP = 0b0000111,  // RV32F
    OP_STOREFP = 0b0100111, // RV32F
    OP_OPFP = 0b1010011,    // RV32F
    OP_MADD = 0b10000111,   // RV32F
    OP_MSUB = 0b10001111,   // RV32F
    OP_NMSUB = 0b10010111,  // RV32F
    OP_NMADD = 0b10011111,  // RV32F
};

#ifdef __cplusplus
extern "C" {
#endif

void Reset(CPU* cpu, uint32_t sp);
CpuResult Decode(CPU* cpu, uint32_t instruction);
CpuResult Fetch(CPU* cpu);
CpuResult HandleTrap(CPU* cpu, Trap trap);
CpuResult Run(CPU* cpu, int count);

#ifdef __cplusplus
}
#endif
