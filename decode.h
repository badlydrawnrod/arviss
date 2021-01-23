#pragma once

#include <stdint.h>

typedef struct Memory Memory;

typedef struct
{
    uint8_t (*ReadByte)(Memory* memory, uint32_t addr);
    uint16_t (*ReadHalfword)(Memory* memory, uint32_t addr);
    uint32_t (*ReadWord)(Memory* memory, uint32_t addr);

    void (*WriteByte)(Memory* memory, uint32_t addr, uint8_t byte);
    void (*WriteHalfword)(Memory* memory, uint32_t addr, uint16_t halfword);
    void (*WriteWord)(Memory* memory, uint32_t addr, uint32_t word);
} MemoryVtbl;

typedef struct FatMem
{
    Memory* mem;
    MemoryVtbl* vtbl;
} FatMem;

static inline uint8_t ReadByte(FatMem mem, uint32_t addr)
{
    return mem.vtbl->ReadByte(mem.mem, addr);
}

static inline uint16_t ReadHalfword(FatMem mem, uint32_t addr)
{
    return mem.vtbl->ReadHalfword(mem.mem, addr);
}

static inline uint32_t ReadWord(FatMem mem, uint32_t addr)
{
    return mem.vtbl->ReadWord(mem.mem, addr);
}

static inline void WriteByte(FatMem mem, uint32_t addr, uint8_t byte)
{
    mem.vtbl->WriteByte(mem.mem, addr, byte);
}

static inline void WriteHalfword(FatMem mem, uint32_t addr, uint16_t halfword)
{
    mem.vtbl->WriteHalfword(mem.mem, addr, halfword);
}

static inline void WriteWord(FatMem mem, uint32_t addr, uint32_t word)
{
    mem.vtbl->WriteWord(mem.mem, addr, word);
}

typedef struct CPU
{
    uint32_t pc;
    uint32_t xreg[32];
    FatMem memory;
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
uint32_t Fetch(CPU* cpu);
void Run(CPU* cpu, int count);

#ifdef __cplusplus
}
#endif
