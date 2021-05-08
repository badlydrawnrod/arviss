#pragma once

#include <stdbool.h>
#include <stdint.h>

// Machine mode traps. See privileged spec, table 3.6: machine cause register (mcause) values after trap.
typedef enum ArvissTrapType
{
    // Non-interrupt traps.
    trINSTRUCTION_MISALIGNED = 0,
    trINSTRUCTION_ACCESS_FAULT = 1,
    trILLEGAL_INSTRUCTION = 2,
    trBREAKPOINT = 3,
    trLOAD_ADDRESS_MISALIGNED = 4,
    trLOAD_ACCESS_FAULT = 5,
    trSTORE_ADDRESS_MISALIGNED = 6,
    trSTORE_ACCESS_FAULT = 7,
    trENVIRONMENT_CALL_FROM_U_MODE = 8,
    trENVIRONMENT_CALL_FROM_S_MODE = 9,
    trRESERVED_10 = 10,
    trENVIRONMENT_CALL_FROM_M_MODE = 11,
    trINSTRUCTION_PAGE_FAULT = 12,
    trRESERVERD_14 = 14,
    trSTORE_PAGE_FAULT = 15,
    trNOT_IMPLEMENTED_YET = 24, // Technically this is the first item reserved for custom use.

    // Interrupts (bit 31 is set).
    trUSER_SOFTWARE_INTERRUPT = 0x80000000 + 0,
    trSUPERVISOR_SOFTWARE_INTERRUPT = 0x80000000 + 1,
    trRESERVED_INT_2 = 0x80000000 + 2,
    trMACHINE_SOFTWARE_INTERRUPT = 0x80000000 + 3,
    trUSER_TIMER_INTERRUPT = 0x80000000 + 4,
    trSUPERVISOR_TIMER_INTERRUPT = 0x80000000 + 5,
    trRESERVED_INT_6 = 0x80000000 + 6,
    trMACHINE_TIMER_INTERRUPT = 0x80000000 + 7
} ArvissTrapType;

typedef struct ArvissTrap
{
    ArvissTrapType mcause; // TODO: interrupts.
    uint32_t mtval;
} ArvissTrap;

typedef enum ArvissResultType
{
    rtOK,
    rtTRAP,
    rtBYTE,
    rtHALFWORD,
    rtWORD,
} ArvissResultType;

typedef struct ArvissResult
{
    union
    {
        uint32_t nothing;
        ArvissTrap trap;
        uint8_t byte;
        uint16_t halfword;
        uint32_t word;
    };
    ArvissResultType type;
} ArvissResult;

#ifdef __cplusplus
extern "C" {
#endif

static inline ArvissResult ArvissMakeOk(void)
{
    ArvissResult r;
    r.type = rtOK;
    return r;
}

static inline ArvissResult ArvissMakeTrap(ArvissTrapType trap, uint32_t value)
{
    ArvissResult r;
    r.type = rtTRAP;
    r.trap.mcause = trap;
    r.trap.mtval = value;
    return r;
}

static inline ArvissResult ArvissMakeByte(uint8_t byte)
{
    ArvissResult r;
    r.type = rtBYTE;
    r.byte = byte;
    return r;
}

static inline ArvissResult ArvissMakeHalfword(uint16_t halfword)
{
    ArvissResult r;
    r.type = rtHALFWORD;
    r.halfword = halfword;
    return r;
}

static inline ArvissResult ArvissMakeWord(uint32_t word)
{
    ArvissResult r;
    r.type = rtWORD;
    r.word = word;
    return r;
}

static inline bool ArvissResultIsTrap(ArvissResult result)
{
    return result.type == rtTRAP;
}

static inline ArvissTrap ArvissResultAsTrap(ArvissResult result)
{
    return result.trap;
}

static inline bool ArvissResultIsByte(ArvissResult result)
{
    return result.type == rtBYTE;
}

static inline uint8_t ArvissResultAsByte(ArvissResult result)
{
    return result.byte;
}

static inline bool ArvissResultIsHalfword(ArvissResult result)
{
    return result.type == rtHALFWORD;
}

static inline uint16_t ArvissResultAsHalfword(ArvissResult result)
{
    return result.halfword;
}

static inline bool ArvissResultIsWord(ArvissResult result)
{
    return result.type == rtWORD;
}

static inline uint32_t ArvissResultAsWord(ArvissResult result)
{
    return result.word;
}

#ifdef __cplusplus
}
#endif
