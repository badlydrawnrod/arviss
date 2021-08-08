#pragma once

#include <stdint.h>

typedef struct MemoryDescriptor
{
    uint32_t start; // The start address of this memory segment in VM memory.
    uint32_t size;  // The size of this memory segment.
    void* data;     // This memory segment's data in host memory.
} MemoryDescriptor;

void LoadElf(const char* filename, MemoryDescriptor* memoryDescriptors, int numDescriptors);
