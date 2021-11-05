#include "arviss.h"
#include "loadelf.h"
#include "mem.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define TTY_STATUS IOBASE
#define TTY_DATA (TTY_STATUS + 1)

static const uint32_t membase = MEMBASE;
static const uint32_t memsize = MEMSIZE;
static const uint32_t rambase = RAMBASE;
static const uint32_t ramsize = RAMSIZE;

static uint8_t Read8(BusToken token, uint32_t addr, BusCode* busCode)
{
    Memory* memory = (Memory*)(token.t);
    if (addr >= membase && addr < membase + memsize)
    {
        return memory->mem[addr - membase];
    }

    if (addr == TTY_STATUS)
    {
        return 0xff; // TODO: return a real status.
    }

    *busCode = bcLOAD_ACCESS_FAULT;
    return 0;
}

static uint16_t Read16(BusToken token, uint32_t addr, BusCode* busCode)
{
    Memory* memory = (Memory*)(token.t);
    if (addr >= membase && addr < membase + memsize - 1)
    {
        // TODO: implement for big-endian ISAs.
        const uint16_t* base = (uint16_t*)&memory->mem[addr - membase];
        return *base;
    }

    *busCode = bcLOAD_ACCESS_FAULT;
    return 0;
}

static uint32_t Read32(BusToken token, uint32_t addr, BusCode* busCode)
{
    Memory* memory = (Memory*)(token.t);
    if (addr >= membase && addr < membase + memsize - 3)
    {
        // TODO: implement for big-endian ISAs.
        const uint32_t* base = (uint32_t*)&memory->mem[addr - membase];
        return *base;
    }

    *busCode = bcLOAD_ACCESS_FAULT;
    return 0;
}

static void Write8(BusToken token, uint32_t addr, uint8_t byte, BusCode* busCode)
{
    Memory* memory = (Memory*)(token.t);
    if (addr >= rambase && addr < rambase + ramsize)
    {
        memory->mem[addr - membase] = byte;
        return;
    }

    if (addr == TTY_DATA)
    {
        putchar(byte);
        return;
    }

    *busCode = bcSTORE_ACCESS_FAULT;
}

static void Write16(BusToken token, uint32_t addr, uint16_t halfword, BusCode* busCode)
{
    Memory* memory = (Memory*)(token.t);
    if (addr >= rambase && addr < rambase + ramsize - 1)
    {
        // TODO: implement for big-endian ISAs.
        uint16_t* base = (uint16_t*)&memory->mem[addr - membase];
        *base = halfword;
        return;
    }

    *busCode = bcSTORE_ACCESS_FAULT;
}

static void Write32(BusToken token, uint32_t addr, uint32_t word, BusCode* busCode)
{
    Memory* memory = (Memory*)(token.t);
    if (addr >= rambase && addr < rambase + ramsize - 3)
    {
        // TODO: implement for big-endian ISAs.
        uint32_t* base = (uint32_t*)&memory->mem[addr - membase];
        *base = word;
        return;
    }

    *busCode = bcSTORE_ACCESS_FAULT;
}

static void ZeroMem(ElfToken token, uint32_t addr, uint32_t len)
{
    uint8_t* target = token.t;
    memset(target + addr, 0, len);
}

static void WriteV(ElfToken token, uint32_t addr, void* src, uint32_t len)
{
    uint8_t* target = token.t;
    memcpy(target + addr, src, len);
}

int main(void)
{
    Memory memory;
    ArvissCpu cpu;

    const char* filename = "../../../../examples/hello_world/arviss/bin/hello";

    if (LoadElf(filename,
                &(ElfLoaderConfig){.token = {&memory.mem},
                                   .zeroMemFn = ZeroMem,
                                   .writeMemFn = WriteV,
                                   .targetSegments = (ElfSegmentDescriptor[]){{.start = ROM_START, .size = ROMSIZE},
                                                                              {.start = RAMBASE, .size = RAMSIZE}},
                                   .numSegments = 2}))
    {
        printf("--- Failed to load %s\n", filename);
        return -1;
    }

    // Run the program, n instructions at a time.
    ArvissInit(&cpu,
               &(Bus){.token = {&memory},
                      .Read8 = Read8,
                      .Read16 = Read16,
                      .Read32 = Read32,
                      .Write8 = Write8,
                      .Write16 = Write16,
                      .Write32 = Write32});
    ArvissResult result = ArvissMakeOk();
    while (!ArvissResultIsTrap(result))
    {
        result = ArvissRun(&cpu, 100000);
    }

    // The exit code (assuming that it exited) is in x10.
    printf("--- Program finished with exit code %d\n", ArvissReadXReg(&cpu, 10));

    return 0;
}
