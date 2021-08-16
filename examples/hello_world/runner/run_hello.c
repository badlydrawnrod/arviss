#include "arviss.h"
#include "loadelf.h"
#include "mem.h"

#include <stdio.h>

uint8_t ReadByte(BusToken token, uint32_t addr, MemoryCode* mc)
{
    ArvissMemory* mem = (ArvissMemory*)(token.t);
    return ArvissReadByte(mem, addr, mc);
}

uint16_t ReadHalfword(BusToken token, uint32_t addr, MemoryCode* mc)
{
    ArvissMemory* mem = (ArvissMemory*)(token.t);
    return ArvissReadHalfword(mem, addr, mc);
}

uint32_t ReadWord(BusToken token, uint32_t addr, MemoryCode* mc)
{
    ArvissMemory* mem = (ArvissMemory*)(token.t);
    return ArvissReadWord(mem, addr, mc);
}

void WriteByte(BusToken token, uint32_t addr, uint8_t byte, MemoryCode* mc)
{
    ArvissMemory* mem = (ArvissMemory*)(token.t);
    ArvissWriteByte(mem, addr, byte, mc);
}

void WriteHalfword(BusToken token, uint32_t addr, uint16_t halfword, MemoryCode* mc)
{
    ArvissMemory* mem = (ArvissMemory*)(token.t);
    ArvissWriteHalfword(mem, addr, halfword, mc);
}

void WriteWord(BusToken token, uint32_t addr, uint32_t word, MemoryCode* mc)
{
    ArvissMemory* mem = (ArvissMemory*)(token.t);
    ArvissWriteWord(mem, addr, word, mc);
}

int main(void)
{
    ArvissMemory memory;
    Bus bus = {.token = {&memory},
               .ReadByte = ReadByte,
               .ReadHalfword = ReadHalfword,
               .ReadWord = ReadWord,
               .WriteByte = WriteByte,
               .WriteHalfword = WriteHalfword,
               .WriteWord = WriteWord};
    ArvissCpu* cpu = ArvissCreate(&bus);

    MemoryDescriptor memoryDesc[] = {{.start = ROM_START, .size = ROMSIZE, .data = memory.mem + ROM_START},
                                     {.start = RAMBASE, .size = RAMSIZE, .data = memory.mem + RAMBASE}};
    const char* filename = "../../../../examples/hello_world/arviss/bin/hello";
    if (LoadElf(filename, memoryDesc, sizeof(memoryDesc) / sizeof(memoryDesc[0])) != ER_OK)
    {
        printf("--- Failed to load %s\n", filename);
        return -1;
    }

    // Run the program, n instructions at a time.
    ArvissInit(cpu, &bus);
    ArvissResult result = ArvissMakeOk();
    while (!ArvissResultIsTrap(result))
    {
        result = ArvissRun(cpu, 100000);
    }

    // The exit code (assuming that it exited) is in x10.
    printf("--- Program finished with exit code %d\n", ArvissReadXReg(cpu, 10));

    ArvissDispose(cpu);

    return 0;
}
