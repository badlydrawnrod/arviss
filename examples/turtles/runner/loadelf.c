#include "loadelf.h"

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

// ELF format references:
// https://en.wikipedia.org/wiki/Executable_and_Linkable_Format
// https://man7.org/linux/man-pages/man5/elf.5.html

#define EI_NIDENT 16

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

typedef uint32_t Elf32_Addr;
typedef uint32_t Elf32_Off;

typedef struct Elf32_Ehdr
{
    unsigned char e_ident[EI_NIDENT];
    uint16_t e_type;
    uint16_t e_machine;
    uint32_t e_version;
    Elf32_Addr e_entry;
    Elf32_Off e_phoff;
    Elf32_Off e_shoff;
    uint32_t e_flags;
    uint16_t e_ehsize;
    uint16_t e_phentsize;
    uint16_t e_phnum;
    uint16_t e_shentsize;
    uint16_t e_shnum;
    uint16_t e_shstrndx;
} Elf32_Ehdr;

typedef struct Elf32_Phdr
{
    uint32_t p_type;
    Elf32_Off p_offset;
    Elf32_Addr p_vaddr;
    Elf32_Addr p_paddr;
    uint32_t p_filesz;
    uint32_t p_memsz;
    uint32_t p_flags;
    uint32_t p_align;
} Elf32_Phdr;

typedef struct Elf32_Shdr
{
    uint32_t sh_name;
    uint32_t sh_type;
    uint32_t sh_flags;
    Elf32_Addr sh_addr;
    Elf32_Off sh_offset;
    uint32_t sh_size;
    uint32_t sh_link;
    uint32_t sh_info;
    uint32_t sh_addralign;
    uint32_t sh_entsize;
} Elf32_Shdr;

void LoadElf(const char* filename, MemoryDescriptor* memoryDescriptors, int numDescriptors)
{
    FILE* fp = NULL;

    if (memoryDescriptors == NULL)
    {
        goto finish;
    }

    if (numDescriptors < 0)
    {
        goto finish;
    }

    fp = fopen(filename, "rb");
    if (!fp)
    {
        goto finish;
    }

    // Get the size of the file.
    if (fseek(fp, 0, SEEK_END) != 0)
    {
        goto finish;
    }
    long here = ftell(fp);
    if (here < 0)
    {
        goto finish;
    }
    size_t fileSize = here;

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

    // If we reach here then we have a 32-bit RISC-V executable that we have a good chance of being able to load.

    // Iterate through the program headers, looking for loadable segments.
    bool entryPointValid = false;
    for (uint16_t i = 0; i < header.e_phnum; i++)
    {
        // Go to this program header's entry in the program header table.
        if (fseek(fp, header.e_phoff + i * header.e_phentsize, SEEK_SET) != 0)
        {
            goto finish;
        }

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

        if (phdr.p_memsz != 0)
        {
            bool targetSegmentFound = false;
            for (MemoryDescriptor* m = memoryDescriptors; m < memoryDescriptors + numDescriptors; m++)
            {
                if (phdr.p_vaddr < m->start || phdr.p_vaddr + phdr.p_memsz >= m->start + m->size)
                {
                    continue;
                }
                targetSegmentFound = true;
                entryPointValid = entryPointValid || (header.e_entry >= m->start && header.e_entry < m->start + m->size);

                // Check that the memory actually points somewhere.
                if (!m->data)
                {
                    goto finish;
                }

                // Go to the segment's file image.
                if (fseek(fp, phdr.p_offset, SEEK_SET) != 0)
                {
                    goto finish;
                }

                // Zero the target memory.
                void* target = (char*)m->data + (phdr.p_vaddr - m->start);
                memset(target, 0, phdr.p_memsz);

                // Load the image.
                if (fread(target, 1, phdr.p_filesz, fp) != phdr.p_filesz)
                {
                    goto finish;
                }

                break;
            }

            // Bail if no memory could be found suitable to load this segment.
            if (!targetSegmentFound)
            {
                goto finish;
            }
        }
    }

    // Bail if the program's entry point isn't valid.
    if (!entryPointValid)
    {
        goto finish;
    }

    // If we get here then we have a valid 32-bit RISC-V executable that we can deal with.
    puts("\nSuccessfully loaded RISC-V executable.");

finish:
    if (fp)
    {
        fclose(fp);
    }
}

int main(void)
{
    char rom[0x4000];
    char ram[0x4000];
    MemoryDescriptor memory[] = {{.start = 0x0, .size = 0x4000, .data = rom}, {.start = 0x4000, .size = 0x4000, .data = ram}};
    const char* filename = "examples/turtles/arviss/build/turtle";
    LoadElf(filename, memory, sizeof(memory) / sizeof(memory[0]));

    return 0;
}
