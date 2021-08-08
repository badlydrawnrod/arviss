#include "loadelf.h"

#include <stdio.h>

#define EI_MAG0 0
#define EI_MAG1 1
#define EI_MAG2 2
#define EI_MAG3 3
#define EI_CLASS 4
#define EI_DATA 5
#define EI_VERSION 6

#define ET_EXEC 2
#define EM_RISCV 0xf3

#define PT_LOAD 1

void LoadElf(const char* filename)
{
    FILE* fp = fopen(filename, "rb");
    if (!fp)
    {
        goto finish;
    }

    // Get the size of the file.
    if (fseek(fp, 0, SEEK_END) != 0)
    {
        goto finish;
    }
    size_t fileSize = ftell(fp);

    //  Bail if the file is too small.
    if (fileSize < sizeof(Elf32_Ehdr))
    {
        goto finish;
    }

    // Return to the beginning of the file.
    if (fseek(fp, 0, SEEK_SET) != 0)
    {
        goto finish;
    }

    // --- Check the ELF header ----------------------------------------------------------------------------------------------------

    // Read the ELF header.
    Elf32_Ehdr header;
    size_t itemsRead = fread(&header, sizeof(header), 1, fp);
    if (itemsRead != 1)
    {
        goto finish;
    }

    // Check the magic number.
    if (header.e_ident[EI_MAG0] != '\x7f' || header.e_ident[EI_MAG1] != 'E' || header.e_ident[EI_MAG2] != 'L'
        || header.e_ident[EI_MAG3] != 'F')
    {
        goto finish;
    }

    // Check that it's 32-bit.
    if (header.e_ident[EI_CLASS] != 1)
    {
        goto finish;
    }

    // Check that it's two's complement, little-endian.
    if (header.e_ident[EI_DATA] != 1)
    {
        goto finish;
    }

    // Check the ident version.
    if (header.e_ident[EI_VERSION] != 1)
    {
        goto finish;
    }

    // If we reach here then we're happy with the e_ident section.

    // Check that it's an executable.
    if (header.e_type != ET_EXEC)
    {
        goto finish;
    }

    // Check that it's for RISC-V.
    if (header.e_machine != EM_RISCV)
    {
        goto finish;
    }

    // Check the version.
    if (header.e_version != 1)
    {
        goto finish;
    }

    // Check that the size of a program header entry is what we expect.
    if (header.e_phentsize != sizeof(Elf32_Phdr))
    {
        goto finish;
    }

    // Check that the program header table is beyond the ELF header and within the file.
    if (header.e_phoff < sizeof(header) || header.e_phoff + header.e_phentsize * header.e_phnum > fileSize)
    {
        goto finish;
    }

    // Check that the size of a section header entry is what we expect.
    if (header.e_shentsize != sizeof(Elf32_Shdr))
    {
        goto finish;
    }

    // Check that the section header table is beyond the ELF header and within the file.
    if (header.e_shoff < sizeof(header) || header.e_shoff + header.e_shentsize * header.e_shnum > fileSize)
    {
        goto finish;
    }

    // If we reach here then we know that we have a 32-bit RISC-V executable that we have a good chance of being able to load.

    printf("     Program Entry point: 0x%08x\n", header.e_entry);

    // --- Check the program headers -----------------------------------------------------------------------------------------------

    // Go to the start of the program header table.
    if (fseek(fp, header.e_phoff, SEEK_SET) != 0)
    {
        goto finish;
    }

    for (uint16_t i = 0; i < header.e_phnum; i++)
    {
        Elf32_Phdr phdr;
        if (fread(&phdr, sizeof(phdr), 1, fp) != 1)
        {
            goto finish;
        }

        // Skip over anything that isn't a loadable segment.
        if (phdr.p_type != PT_LOAD)
        {
            continue;
        }

        // Check that this segment's file image is beyond the ELF header and within the file.
        if (phdr.p_offset < sizeof(Elf32_Ehdr) || phdr.p_offset + phdr.p_filesz > fileSize)
        {
            goto finish;
        }

        // Check that its size in memory is at least as large as its file image.
        if (phdr.p_memsz < phdr.p_filesz)
        {
            goto finish;
        }

        // TODO: Check that this segment will fit in memory.

        printf("\nLoadable segment\n");
        printf("\t     File offset: 0x%08x\n", phdr.p_offset);
        printf("\t      Image size: %d bytes\n", phdr.p_filesz);
        printf("\t     Memory size: %d bytes\n", phdr.p_memsz);
        printf("\tPhysical address: 0x%08x\n", phdr.p_paddr);
        printf("\t Virtual address: 0x%08x\n", phdr.p_vaddr);
    }

    // If we get here then we have a valid 32-bit RISC-V executable that we can deal with.

    puts("\nWe have a valid RISC-V executable.");

finish:
    if (fp)
    {
        fclose(fp);
    }
}

int main(void)
{
    const char* filename = "examples/turtles/arviss/build/turtle";
    LoadElf(filename);

    return 0;
}
