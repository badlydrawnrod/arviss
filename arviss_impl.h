#pragma once

#include "arviss.h"
#include "memory.h"
#include "result.h"

#include <stdint.h>

typedef struct DecodedInstruction DecodedInstruction;

typedef void (*ExecFn)(ArvissCpu* cpu, const DecodedInstruction* ins);

struct DecodedInstruction
{
    ExecFn opcode;
    union
    {
        struct
        {
            uint32_t cacheLine; // The instruction's cache line.
            uint32_t index;     // The instruction's index in the cache line.
        } fdr;

        struct
        {
            uint8_t rd;  // Destination register.
            int32_t imm; // Immediate operand.
        } rd_imm;

        struct
        {
            uint8_t rd;  // Destination register.
            uint8_t rs1; // Source register.
        } rd_rs1;

        struct
        {
            uint8_t rd;  // Destination register.
            uint8_t rs1; // Source register.
            int32_t imm; // Immediate operand.
        } rd_rs1_imm;

        struct
        {
            uint8_t rd;  // Destination register.
            uint8_t rs1; // First source register.
            uint8_t rs2; // Second source register.
        } rd_rs1_rs2;

        struct
        {
            uint8_t rs1; // First source register.
            uint8_t rs2; // Second source register.
            int32_t imm; // Immediate operand.
        } rs1_rs2_imm;

        struct
        {
            uint8_t rd;  // Destination register.
            uint8_t rs1; // First source register.
            uint8_t rs2; // Second source register.
            uint8_t rs3; // Third source register.
            uint8_t rm;  // Rounding mode.
        } rd_rs1_rs2_rs3_rm;

        struct
        {
            uint8_t rd;  // Destination register.
            uint8_t rs1; // Source register.
            uint8_t rm;  // Rounding mode.
        } rd_rs1_rm;

        struct
        {
            uint8_t rd;  // Destination register.
            uint8_t rs1; // First source register.
            uint8_t rs2; // Second source register.
            uint8_t rm;  // Rounding mode.
        } rd_rs1_rs2_rm;

        uint32_t ins; // Instruction.
    };
};

#define CACHE_LINES 64
#define CACHE_LINE_LENGTH 32

typedef struct DecodedInstructionCache
{
    struct CacheLine
    {
        uint32_t owner;                                     // The address that owns this cache line.
        DecodedInstruction instructions[CACHE_LINE_LENGTH]; // The cache line itself.
        bool isValid;                                       // True if the cache line is valid.
    } line[CACHE_LINES];
} DecodedInstructionCache;

struct ArvissCpu
{
    ArvissResult result;           // The result of the last operation.
    BusCode mc;                 // The result of the last memory operation.
    uint32_t pc;                   // The program counter.
    uint32_t xreg[32];             // Regular registers, x0-x31.
    uint32_t mepc;                 // The machine exception program counter.
    uint32_t mcause;               // The machine cause register.
    uint32_t mtval;                // The machine trap value register.
    float freg[32];                // Floating point registers, f0-f31.
    uint32_t fcsr;                 // Floating point control and status register.
    Bus* bus;                      // The CPU's address bus.
    DecodedInstructionCache cache; // The decoded instruction cache.
};

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __cplusplus
}
#endif
