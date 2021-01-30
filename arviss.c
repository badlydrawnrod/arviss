#include "decode.h"
#include "smallmem.h"

#include <stdint.h>
#include <stdio.h>

int main()
{
    Memory memory;
    CPU cpu = { .memory = smallmem_Init(&memory) };
    Reset(&cpu, RAMBASE + RAMSIZE);

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
