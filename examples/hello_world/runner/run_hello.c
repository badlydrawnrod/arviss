#include "arviss.h"
#include "loadelf.h"
#include "mem.h"

#include <stdio.h>

int main(void)
{
    ArvissCpu* cpu = ArvissCreate();
    ArvissReset(cpu);
    ArvissMemory* memory = ArvissGetMemory(cpu);

    MemoryDescriptor memoryDesc[] = {{.start = ROM_START, .size = ROMSIZE, .data = memory->mem + ROM_START},
                                     {.start = RAMBASE, .size = RAMSIZE, .data = memory->mem + RAMBASE}};
    const char* filename = "../../../../examples/hello_world/arviss/build/hello";
    if (LoadElf(filename, memoryDesc, sizeof(memoryDesc) / sizeof(memoryDesc[0])) != ER_OK)
    {
        printf("--- Failed to load %s\n", filename);
        return -1;
    }

    // Run the program, n instructions at a time.
    ArvissReset(cpu);
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
