#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef enum Syscalls
{
    SYSCALL_EXIT,                // The robot's program has finished.
    SYSCALL_YIELD,               // Give way.
    SYSCALL_GET_MY_POSITION,     // Gets the robot's position in the world.
    SYSCALL_GET_PLAYER_POSITION, // Gets the player's position in the world.
    SYSCALL_FIRE_AT,             // Fires a shot towards the given postion.
    SYSCALL_MOVE_TOWARDS,        // Instructs the robot to move towards the given position.
    SYSCALL_STOP,                // Instructs the robot to stop.
    SYSCALL_RAYCAST_TOWARDS      // Fires a ray towards the given position to detect obstacles.
} Syscalls;

typedef struct RkVector
{
    float x;
    float y;
} RkVector;

// Credit to: https://github.com/lluixhi/musl-riscv/blob/master/arch/riscv32/syscall_arch.h

/*
 * AssemblerTemplate : OutputOperands [ : InputOperands [ : Clobbers ] ]
 *
 */

#define __asm_syscall(...)                                                                                                         \
    asm volatile("ecall\n\t" : "=r"(a0) : __VA_ARGS__ : "memory");                                                                 \
    return a0;

static inline uint32_t syscall0(uint32_t n)
{
    register uint32_t a7 asm("a7") = n;
    register uint32_t a0 asm("a0");
    __asm_syscall("r"(a7))
}

static inline uint32_t syscall1(uint32_t n, uint32_t a)
{
    register uint32_t a7 asm("a7") = n;
    register uint32_t a0 asm("a0") = a;
    __asm_syscall("r"(a7), "r"(a0))
}

static inline uint32_t syscall2(uint32_t n, uint32_t a, uint32_t b)
{
    register uint32_t a7 asm("a7") = n;
    register uint32_t a0 asm("a0") = a;
    register uint32_t a1 asm("a1") = b;
    __asm_syscall("r"(a7), "r"(a0), "r"(a1))
}

static inline uint32_t syscall3(uint32_t n, uint32_t a, uint32_t b, uint32_t c)
{
    register uint32_t a7 asm("a7") = n;
    register uint32_t a0 asm("a0") = a;
    register uint32_t a1 asm("a1") = b;
    register uint32_t a2 asm("a2") = c;
    __asm_syscall("r"(a7), "r"(a0), "r"(a1), "r"(a2))
}

static inline void RkExit(int32_t status)
{
    syscall1(SYSCALL_EXIT, status);
}

static inline void RkYield(void)
{
    syscall0(SYSCALL_YIELD);
}

static inline void RkGetMyPosition(RkVector* v)
{
    syscall1(SYSCALL_GET_MY_POSITION, (uint32_t)v);
}

static inline void RkGetPlayerPosition(RkVector* v)
{
    syscall1(SYSCALL_GET_PLAYER_POSITION, (uint32_t)v);
}

static inline void RkFireAt(RkVector* v)
{
    syscall1(SYSCALL_FIRE_AT, (uint32_t)v);
}

static inline void RkMoveTowards(RkVector* v)
{
    syscall1(SYSCALL_MOVE_TOWARDS, (uint32_t)v);
}

static inline void RkStop(void)
{
    syscall0(SYSCALL_STOP);
}

static inline bool RkRaycastTowards(RkVector* v, float distance)
{
    return syscall2(SYSCALL_RAYCAST_TOWARDS, (uint32_t)v, *(uint32_t*)&distance);
}
