#pragma once

#include <stdint.h>

static inline float U32AsFloat(const uint32_t a)
{
    union
    {
        uint32_t a;
        float b;
    } u;
    u.a = a;
    return u.b;
}

static inline uint32_t FloatAsU32(const float a)
{
    union
    {
        float a;
        uint32_t b;
    } u;
    u.a = a;
    return u.b;
}

static inline uint32_t BoolAsU32(const bool b)
{
    return b ? 1 : 0;
}
