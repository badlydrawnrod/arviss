#pragma once

#include <stdint.h>

static inline float U32AsFloat(const uint32_t a)
{
    union
    {
        uint32_t a;
        float b;
    } x;
    x.a = a;
    return x.b;
}

static inline uint32_t FloatAsU32(const float a)
{
    union
    {
        float a;
        uint32_t b;
    } x;
    x.a = a;
    return x.b;
}
