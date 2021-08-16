/**
 * Arviss - A Risc-V Instruction Set Simulator.
 */
#pragma once

#include "result.h"

#include <stdint.h>

/**
 * A handle to an Arviss CPU.
 */
typedef struct ArvissCpu ArvissCpu;

/**
 * An arbitrary, caller supplied token that an Arviss CPU passes to the bus callbacks.
 */
typedef struct
{
    void* t;
} BusToken;

typedef enum BusCode
{
    bcOK,
    bcLOAD_ACCESS_FAULT,
    bcSTORE_ACCESS_FAULT
} BusCode;

/**
 * Signatures of bus callbacks.
 */
typedef uint8_t (*BusRead8Fn)(BusToken token, uint32_t addr, BusCode* busCode);
typedef uint16_t (*BusRead16Fn)(BusToken token, uint32_t addr, BusCode* busCode);
typedef uint32_t (*BusRead32Fn)(BusToken token, uint32_t addr, BusCode* busCode);
typedef void (*BusWrite8Fn)(BusToken token, uint32_t addr, uint8_t byte, BusCode* busCode);
typedef void (*BusWrite16Fn)(BusToken token, uint32_t addr, uint16_t halfword, BusCode* busCode);
typedef void (*BusWrite32Fn)(BusToken token, uint32_t addr, uint32_t word, BusCode* busCode);

/**
 * The bus is how an Arviss CPU interacts with the rest of the system. It has a number of callbacks, and a caller-supplied token
 * that is passed to them on invocation.
 */
typedef struct Bus
{
    BusToken token;
    BusRead8Fn Read8;
    BusRead16Fn Read16;
    BusRead32Fn Read32;
    BusWrite8Fn Write8;
    BusWrite16Fn Write16;
    BusWrite32Fn Write32;
} Bus;

typedef struct DecodedInstruction DecodedInstruction;

typedef void (*ExecFn)(ArvissCpu* cpu, const DecodedInstruction* ins);

struct DecodedInstruction
{
    ExecFn opcode; // The function that will execute this instruction.
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
    BusCode mc;                    // The result of the last memory operation.
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

/**
 * Initialises an Arviss CPU.
 * @param cpu the CPU.
 * @param bus the bus that the CPU should use to interact with the rest of the system.
 */
void ArvissInit(ArvissCpu* cpu, Bus* bus);

/**
 * Resets the given Arviss CPU.
 * @param cpu the CPU to reset.
 */
void ArvissReset(ArvissCpu* cpu);

/**
 * Decodes and executes a single Arviss instruction.
 * @param cpu the CPU.
 * @param instruction the instruction to execute.
 * @return an ArvissResult indicating the state of the CPU after executing the instruction.
 */
ArvissResult ArvissExecute(ArvissCpu* cpu, uint32_t instruction);

/**
 * Runs count instructions on the CPU.
 * @param cpu the CPU.
 * @param count how many instructions to run.
 * @return an ArvissResult indicating the state of the CPU after attempting to run count instructions.
 */
ArvissResult ArvissRun(ArvissCpu* cpu, int count);

/**
 * Reads the given X register.
 * @param cpu the CPU.
 * @param reg which X register to read (0 - 31).
 * @return the content of the X register.
 */
uint32_t ArvissReadXReg(ArvissCpu* cpu, int reg);

/**
 * Writes to the given X register.
 * @param cpu the CPU.
 * @param reg which X register to write to (0 - 31).
 * @param value the value to write.
 */
void ArvissWriteXReg(ArvissCpu* cpu, int reg, uint32_t value);

/**
 * Reads the given F register.
 * @param cpu the CPU.
 * @param reg which F register to read (0 - 31).
 * @return the content of the F register.
 */
float ArvissReadFReg(ArvissCpu* cpu, int reg);

/**
 * Writes to the given F register.
 * @param cpu the CPU.
 * @param reg which F register to write to (0 - 31).
 * @param value the value to write.
 */
void ArvissWriteFReg(ArvissCpu* cpu, int reg, float value);

/**
 * Performs an MRET instruction on the CPU. Use this when returning from a machine-mode trap.
 * @param cpu the CPU.
 */
void ArvissMret(ArvissCpu* cpu);

#ifdef __cplusplus
}
#endif
