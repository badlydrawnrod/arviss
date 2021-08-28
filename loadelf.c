#include "loadelf.h"

#include <stdbool.h>
#include <stdio.h>

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

ElfResult LoadElf(const char* filename, ElfToken token, ElfFillNFn fillFn, ElfWriteVFn writeFn, MemoryDescriptor* memoryDescriptors,
                  int numDescriptors)
{
    ElfResult er = ER_BAD_ELF;

    FILE* fp = NULL;

    if (memoryDescriptors == NULL)
    {
        er = ER_INVALID_ARGUMENT;
        goto finish;
    }

    if (numDescriptors < 0)
    {
        er = ER_INVALID_ARGUMENT;
        goto finish;
    }

    fp = fopen(filename, "rb");
    if (!fp)
    {
        er = ER_IO_FAILED;
        goto finish;
    }

    // Get the size of the file.
    if (fseek(fp, 0, SEEK_END) != 0)
    {
        er = ER_IO_FAILED;
        goto finish;
    }
    long here = ftell(fp);
    if (here < 0)
    {
        er = ER_IO_FAILED;
        goto finish;
    }
    size_t fileSize = here;

    //  Bail if the file is too small.
    if (fileSize < sizeof(Elf32_Ehdr))
    {
        er = ER_BAD_ELF;
        goto finish;
    }

    // Return to the beginning of the file.
    if (fseek(fp, 0, SEEK_SET) != 0)
    {
        er = ER_IO_FAILED;
        goto finish;
    }

    // Read the ELF header.
    Elf32_Ehdr header;
    size_t itemsRead = fread(&header, sizeof(header), 1, fp);
    if (itemsRead != 1)
    {
        er = ER_IO_FAILED;
        goto finish;
    }

    // Check the magic number.
    if (header.e_ident[EI_MAG0] != '\x7f' || header.e_ident[EI_MAG1] != 'E' || header.e_ident[EI_MAG2] != 'L'
        || header.e_ident[EI_MAG3] != 'F')
    {
        er = ER_BAD_ELF;
        goto finish;
    }

    // Check that it's 32-bit.
    if (header.e_ident[EI_CLASS] != 1)
    {
        er = ER_NOT_SUPPORTED;
        goto finish;
    }

    // Check that it's two's complement, little-endian.
    if (header.e_ident[EI_DATA] != 1)
    {
        er = ER_NOT_SUPPORTED;
        goto finish;
    }

    // Check the ident version.
    if (header.e_ident[EI_VERSION] != 1)
    {
        er = ER_NOT_SUPPORTED;
        goto finish;
    }

    // If we reach here then we're happy with the e_ident section.

    // Check that it's an executable.
    if (header.e_type != ET_EXEC)
    {
        er = ER_NOT_SUPPORTED;
        goto finish;
    }

    // Check that it's for RISC-V.
    if (header.e_machine != EM_RISCV)
    {
        er = ER_NOT_SUPPORTED;
        goto finish;
    }

    // Check the version.
    if (header.e_version != 1)
    {
        er = ER_NOT_SUPPORTED;
        goto finish;
    }

    // Check that the size of a program header entry is what we expect.
    if (header.e_phentsize != sizeof(Elf32_Phdr))
    {
        er = ER_BAD_ELF;
        goto finish;
    }

    // Check that the program header table is beyond the ELF header and within the file.
    if (header.e_phoff < sizeof(header) || header.e_phoff + header.e_phentsize * header.e_phnum > fileSize)
    {
        er = ER_BAD_ELF;
        goto finish;
    }

    // Check that the size of a section header entry is what we expect.
    if (header.e_shentsize != sizeof(Elf32_Shdr))
    {
        er = ER_BAD_ELF;
        goto finish;
    }

    // Check that the section header table is beyond the ELF header and within the file.
    if (header.e_shoff < sizeof(header) || header.e_shoff + header.e_shentsize * header.e_shnum > fileSize)
    {
        er = ER_BAD_ELF;
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
            er = ER_IO_FAILED;
            goto finish;
        }

        Elf32_Phdr phdr;
        if (fread(&phdr, sizeof(phdr), 1, fp) != 1)
        {
            er = ER_IO_FAILED;
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
            er = ER_BAD_ELF;
            goto finish;
        }

        // Check that its size in memory is at least as large as its file image.
        if (phdr.p_memsz < phdr.p_filesz)
        {
            er = ER_BAD_ELF;
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

                // Go to the segment's file image.
                if (fseek(fp, phdr.p_offset, SEEK_SET) != 0)
                {
                    er = ER_IO_FAILED;
                    goto finish;
                }

                // Zero the target memory.
                uint32_t dstAddr = (phdr.p_vaddr - m->start);
                fillFn(token, dstAddr, phdr.p_memsz, '\0');

                // Load the image.
                uint8_t buf[BUFSIZ];
                uint32_t remaining = phdr.p_filesz;
                size_t ofs = 0;
                while (remaining != 0)
                {
                    const size_t readLen = remaining >= sizeof(buf) ? sizeof(buf) : remaining;
                    size_t amountRead = fread(buf, 1, readLen, fp);
                    if (amountRead == 0)
                    {
                        break;
                    }
                    remaining -= amountRead;
                    // Copy the buffer into the target memory.
                    writeFn(token, dstAddr + ofs, buf, amountRead);
                    ofs += amountRead;
                }

                if (remaining != 0)
                {
                    er = ER_IO_FAILED;
                    goto finish;
                }

                break;
            }

            // Bail if no memory could be found suitable to load this segment.
            if (!targetSegmentFound)
            {
                er = ER_SEGMENT_NOT_IN_MEMORY;
                goto finish;
            }
        }
    }

    // Bail if the program's entry point isn't valid.
    if (!entryPointValid)
    {
        er = ER_ENTRY_POINT_INVALID;
        goto finish;
    }

    // If we get here without encountering an error then we have a valid 32-bit RISC-V executable that we can deal with.
    er = ER_OK;

finish:
    if (fp)
    {
        if (fclose(fp) != 0)
        {
            // Yes, even closing a file can fail.
            er = ER_IO_FAILED;
        }
    }

    return er;
}
