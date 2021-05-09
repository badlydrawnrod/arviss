#include "arviss.h"
#include "mem.h"

#include <stdint.h>
#include <stdio.h>

int main(void)
{
    ArvissMemory memory;
    ArvissCpu cpu = {.memory = MemInit(&memory)};
    ArvissReset(&cpu, 0);

    printf("--- Loading program and running it\n");
    FILE* fp = NULL;
    errno_t err = fopen_s(&fp, "../hello_world/hello.bin", "rb");
    if (err != 0)
    {
        return err;
    }
    size_t count = fread(memory.mem, 1, sizeof(memory.mem), fp);
    printf("Read %d bytes\n", count);
    fclose(fp);

    // Run the program, n instructions at a time.
    ArvissResult result = ArvissMakeOk();
    while (!ArvissResultIsTrap(result))
    {
        result = ArvissRun(&cpu, 100000);
    }

    // The exit code (assuming that it exited) is in x10.
    printf("--- Program finished with exit code %d\n", cpu.xreg[10]);

    return 0;
}
