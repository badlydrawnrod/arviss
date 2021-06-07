#include "arviss.h"
#include "mem.h"

#include <errno.h>
#include <stdint.h>
#include <stdio.h>

int main(void)
{
    ArvissMemory memory;
    ArvissCpu cpu = {.memory = MemInit(&memory)};
    ArvissReset(&cpu, 0);

    printf("--- Loading program and running it\n");
    FILE* fp = fopen("../../../../examples/hello_world/arviss/bin/hello.bin", "rb");
    if (fp == NULL)
    {
        return -1;
    }
    size_t count = fread(memory.mem, 1, sizeof(memory.mem), fp);
    printf("Read %zd bytes\n", count);
    fclose(fp);

    // Run the program, n instructions at a time.
    ArvissReset(&cpu, 0);
    ArvissResult result = ArvissMakeOk();
    while (!ArvissResultIsTrap(result))
    {
        result = ArvissRun(&cpu, 100000);
    }

    // The exit code (assuming that it exited) is in x10.
    printf("--- Program finished with exit code %d\n", cpu.xreg[10]);

    return 0;
}
