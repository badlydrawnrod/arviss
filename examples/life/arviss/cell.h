#include "syscalls.h"

static inline int CountNeighbours(void)
{
    return (int)syscall0(SYSCALL_COUNT);
}

static inline bool GetState(void)
{
    return (bool)syscall0(SYSCALL_GET_STATE);
}

static inline void SetState(bool state)
{
    syscall1(SYSCALL_SET_STATE, (uint32_t)state);
}
