#pragma once

#include "memory.h"

#define RAMBASE 0
#define RAMSIZE 0x8000

struct ArvissMemory
{
    uint8_t ram[RAMSIZE];
};
