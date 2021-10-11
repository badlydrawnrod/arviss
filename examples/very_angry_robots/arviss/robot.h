#include "syscalls.h"

typedef struct Vector
{
    float x;
    float y;
} Vector;

static inline void Exit(int32_t status)
{
    syscall1(SYSCALL_EXIT, status);
}

static inline void Yield(void)
{
    syscall0(SYSCALL_YIELD);
}

static inline void GetMyPosition(Vector* v)
{
    syscall1(SYSCALL_GET_MY_POSITION, (uint32_t)v);
}

static inline void GetPlayerPosition(Vector* v)
{
    syscall1(SYSCALL_GET_PLAYER_POSITION, (uint32_t)v);
}

static inline void FireAt(Vector* v)
{
    syscall1(SYSCALL_FIRE_AT, (uint32_t)v);
}

static inline void MoveTowards(Vector* v)
{
    syscall1(SYSCALL_MOVE_TOWARDS, (uint32_t)v);
}

static inline void Stop(void)
{
    syscall0(SYSCALL_STOP);
}

static inline bool RaycastTowards(Vector* v, float distance)
{
    return syscall2(SYSCALL_RAYCAST_TOWARDS, (uint32_t)v, *(uint32_t*)&distance);
}
