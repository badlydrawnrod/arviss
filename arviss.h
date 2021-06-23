/**
 * Arviss - A Risc-V Instruction Set Simulator.
 */
#pragma once

#include "memory.h"
#include "result.h"

#include <stdint.h>

/**
 * A handle to an Arviss CPU.
 */
typedef struct ArvissCpu ArvissCpu;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Describes how an Arviss CPU should be created.
 */
typedef struct ArvissDesc
{
    ArvissMemoryTrait memory; // The CPU's memory.
} ArvissDesc;

/**
 * Creates an Arviss CPU from the given descriptor.
 * @param desc a descriptor that is used to configure the CPU.
 * @return a CPU if successful, otherwise NULL.
 */
ArvissCpu* ArvissCreate(const ArvissDesc* desc);

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
 * Returns the CPU's memory.
 * @param cpu the CPU.
 * @return the memory attached to the CPU.
 */
ArvissMemoryTrait ArvissGetMemory(ArvissCpu* cpu);

/**
 * Performs an MRET instruction on the CPU. Use this when returning from a machine-mode trap.
 * @param cpu the CPU.
 */
void ArvissMret(ArvissCpu* cpu);

#ifdef __cplusplus
}
#endif
