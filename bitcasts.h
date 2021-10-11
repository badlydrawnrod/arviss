#pragma once

#include <stdint.h>

static inline float U32AsFloat(const uint32_t a)
{
    return *(float*)&a;
}

static inline uint32_t FloatAsU32(const float a)
{
    return *(uint32_t*)&a;
}

static inline uint32_t BoolAsU32(const bool b)
{
    return b ? 1 : 0;
}
