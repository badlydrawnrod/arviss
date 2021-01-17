#pragma once

#include <stdint.h>

typedef struct CPU
{
    uint32_t pc;
    uint32_t xreg[32];

    uint8_t (*ReadByte)(uint32_t addr);
    uint16_t (*ReadHalfword)(uint32_t addr);
    uint32_t (*ReadWord)(uint32_t addr);

    void (*WriteByte)(uint32_t addr, uint8_t byte);
    void (*WriteHalfword)(uint32_t addr, uint16_t halfword);
    void (*WriteWord)(uint32_t addr, uint32_t word);
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
    OP_SYSTEM = 0b1110011
};

#ifdef __cplusplus
extern "C" {
#endif

void Decode(CPU* cpu, uint32_t instruction);

#ifdef __cplusplus
}
#endif

