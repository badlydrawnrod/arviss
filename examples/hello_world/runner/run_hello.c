#include "arviss.h"
#include "mem.h"

#include <stdint.h>
#include <stdio.h>

int main(void)
{
    ArvissMemory memory;
    ArvissCpu* cpu = ArvissCreate(&(ArvissDesc){.memory = &memory});
    ArvissReset(cpu);

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
