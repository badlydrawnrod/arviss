#include "decode.h"

#include <stdint.h>
#include <stdio.h>

#define RAMSIZE 0x8000

const uint32_t rambase = 0x0;
const uint32_t ramsize = RAMSIZE;

struct Memory
{
    uint8_t ram[RAMSIZE];
};

static uint8_t arviss_ReadByte(Memory* memory, uint32_t addr);
static uint16_t arviss_ReadHalfword(Memory* memory, uint32_t addr);
static uint32_t arviss_ReadWord(Memory* memory, uint32_t addr);
static void arviss_WriteByte(Memory* memory, uint32_t addr, uint8_t byte);
static void arviss_WriteHalfword(Memory* memory, uint32_t addr, uint16_t halfword);
static void arviss_WriteWord(Memory* memory, uint32_t addr, uint32_t word);

static MemoryVtbl vtbl = {arviss_ReadByte,  arviss_ReadHalfword,  arviss_ReadWord,
                          arviss_WriteByte, arviss_WriteHalfword, arviss_WriteWord};

static uint8_t arviss_ReadByte(Memory* memory, uint32_t addr)
{
    if (addr >= rambase && addr < rambase + ramsize)
    {
        return memory->ram[addr - rambase];
    }
    return 0;
}

static uint16_t arviss_ReadHalfword(Memory* memory, uint32_t addr)
{
    if (addr >= rambase && addr < rambase + ramsize - 1)
    {
        return memory->ram[addr - rambase] | (memory->ram[addr + 1 - rambase] << 8);
    }
    return 0;
}

static uint32_t arviss_ReadWord(Memory* memory, uint32_t addr)
{
    if (addr >= rambase && addr < rambase + ramsize - 3)
    {
        return memory->ram[addr - rambase] | (memory->ram[addr + 1 - rambase] << 8) | (memory->ram[addr + 2 - rambase] << 16)
                | (memory->ram[addr + 3 - rambase] << 24);
    }
    return 0;
}

static void arviss_WriteByte(Memory* memory, uint32_t addr, uint8_t byte)
{
    if (addr >= rambase && addr < rambase + ramsize)
    {
        memory->ram[addr - rambase] = byte;
    }
}

static void arviss_WriteHalfword(Memory* memory, uint32_t addr, uint16_t halfword)
{
    if (addr >= rambase && addr < rambase + ramsize - 1)
    {
        memory->ram[addr - rambase] = halfword & 0xff;
        memory->ram[addr + 1 - rambase] = (halfword >> 8) & 0xff;
    }
}

static void arviss_WriteWord(Memory* memory, uint32_t addr, uint32_t word)
{
    if (addr >= rambase && addr < rambase + ramsize - 3)
    {
        memory->ram[addr - rambase] = word & 0xff;
        memory->ram[addr + 1 - rambase] = (word >> 8) & 0xff;
        memory->ram[addr + 2 - rambase] = (word >> 16) & 0xff;
        memory->ram[addr + 3 - rambase] = (word >> 24) & 0xff;
    }
}

int main()
{
    Memory memory;
    CPU cpu = {.pc = 0, .xreg[2] = rambase + ramsize, .memory = {.vtbl = &vtbl, .mem = &memory}};

    printf("--- Loading program and running it\n");
    FILE* fp = NULL;
    errno_t err = fopen_s(&fp, "../boot/prog.bin", "rb");
    if (err != 0)
    {
        return err;
    }
    size_t count = fread(memory.ram, 1, sizeof(memory.ram), fp);
    printf("Read %d bytes\n", count);
    fclose(fp);

    Run(&cpu, 11150);

    printf("x10 (the result) is %d\n", cpu.xreg[10]);

    return 0;
}
