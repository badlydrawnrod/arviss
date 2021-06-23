#pragma once

#include "arviss.h"
#include "memory.h"
#include "result.h"

#include <stdint.h>

typedef struct DecodedInstruction DecodedInstruction;

typedef struct ArvissCpu ArvissCpu;

typedef void (*ExecFn)(ArvissCpu* cpu, const DecodedInstruction* ins);

struct DecodedInstruction
{
    ExecFn opcode;
    union
    {
        struct
        {
            uint32_t cacheLine;
            uint32_t index;
        } fdr;

        struct
        {
            uint8_t rd;
            int32_t imm;
        } rd_imm;

        struct
        {
            uint8_t rd;
            uint8_t rs1;
        } rd_rs1;

        struct
        {
            uint8_t rd;
            uint8_t rs1;
            int32_t imm;
        } rd_rs1_imm;

        struct
        {
            uint8_t rd;
            uint8_t rs1;
            uint8_t rs2;
        } rd_rs1_rs2;

        struct
        {
            uint8_t rs1;
            uint8_t rs2;
            int32_t imm;
        } rs1_rs2_imm;

        struct
        {
            uint8_t rd;
            uint8_t rs1;
            uint8_t rs2;
            uint8_t rs3;
            uint8_t rm;
        } rd_rs1_rs2_rs3_rm;

        struct
        {
            uint8_t rd;
            uint8_t rs1;
            uint8_t rm;
        } rd_rs1_rm;

        struct
        {
            uint8_t rd;
            uint8_t rs1;
            uint8_t rs2;
            uint8_t rm;
        } rd_rs1_rs2_rm;

        uint32_t ins;
    };
};

#define CACHE_LINES 64
#define CACHE_LINE_LENGTH 32

typedef struct DecodedCache
{
    struct CacheLine
    {
        uint32_t owner;
        DecodedInstruction instructions[CACHE_LINE_LENGTH];
        bool isValid;
    } line[CACHE_LINES];
} DecodedCache;

typedef struct ArvissCpu ArvissCpu;

struct ArvissCpu
{
    ArvissResult result; // The result of the last operation.
    MemoryCode mc;       // The result of the last memory operation.
    uint32_t pc;         // The program counter.
    uint32_t xreg[32];   // Regular registers, x0-x31.
    uint32_t mepc;       // The machine exception program counter.
    uint32_t mcause;     // The machine cause register.
    uint32_t mtval;      // The machine trap value register.
    float freg[32];      // Floating point registers, f0-f31.
    uint32_t fcsr;       // Floating point control and status register.
    DecodedCache cache;  // The decoded instruction cache.
    ArvissMemoryTrait memory;
};

#ifdef __cplusplus
extern "C" {
#endif

void ArvissReset(ArvissCpu* cpu, uint32_t sp);
ArvissResult ArvissExecute(ArvissCpu* cpu, uint32_t instruction);
ArvissResult ArvissRun(ArvissCpu* cpu, int count);

#ifdef __cplusplus
}
#endif
