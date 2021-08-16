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

typedef struct
{
    void* t;
} BusToken;

typedef enum MemoryCode
{
    mcOK,
    mcLOAD_ACCESS_FAULT,
    mcSTORE_ACCESS_FAULT
} MemoryCode;

typedef uint8_t (*BusRead8Fn)(BusToken token, uint32_t addr, MemoryCode* mc);
typedef uint16_t (*BusRead16Fn)(BusToken token, uint32_t addr, MemoryCode* mc);
typedef uint32_t (*BusRead32Fn)(BusToken token, uint32_t addr, MemoryCode* mc);
typedef void (*BusWrite8Fn)(BusToken token, uint32_t addr, uint8_t byte, MemoryCode* mc);
typedef void (*BusWrite16Fn)(BusToken token, uint32_t addr, uint16_t halfword, MemoryCode* mc);
typedef void (*BusWrite32Fn)(BusToken token, uint32_t addr, uint32_t word, MemoryCode* mc);

/**
 * The bus is how an Arviss CPU interacts with the rest of the system.
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
 * Creates an Arviss CPU.
 * @return a CPU if successful, otherwise NULL.
 */
ArvissCpu* ArvissCreate(Bus* bus);

/**
 * Disposes of the given Arviss CPU and frees its resources.
 * @param cpu the CPU to dispose of.
 */
void ArvissDispose(ArvissCpu* cpu);

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
