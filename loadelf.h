#pragma once

#include <stdint.h>

typedef struct MemoryDescriptor
{
    uint32_t start; // The start address of this memory segment in VM memory.
    uint32_t size;  // The size of this memory segment.
    void* data;     // This memory segment's data in host memory.
} MemoryDescriptor;

typedef enum ElfResult
{
    ER_OK = 0,                // The ELF file was loaded successfully.
    ER_INVALID_ARGUMENT,      // The caller passed a bad argument.
    ER_IO_FAILED,             // An I/O operation failed while reading the ELF file.
    ER_BAD_ELF,               // The ELF file is badly formatted in some way.
    ER_NOT_SUPPORTED,         // The loader doesn't support some aspect of the ELF file, e.g., it isn't RISC-V.
    ER_SEGMENT_NOT_IN_MEMORY, // A loadable segment doesn't correspond to any memory location supplied by the caller.
    ER_ENTRY_POINT_INVALID,   // The entry point doesn't correspond to any memory location supplied by the caller.
} ElfResult;

ElfResult LoadElf(const char* filename, MemoryDescriptor* memoryDescriptors, int numDescriptors);