#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef enum Syscalls
{
    SYSCALL_EXIT,           // The turtle's program has finished.
    SYSCALL_HOME,           // Tells the turtle to return to the origin, facing up.
    SYSCALL_AHEAD,          // Tells the turtle to move ahead (or behind if negative) by the given amount.
    SYSCALL_TURN,           // Tells the turtle to turn right (or left if negative) by the given amount.
    SYSCALL_GOTO,           // Tells the turtle to teleport to the given location.
    SYSCALL_SET_PEN_STATE,  // Sets the pen state to up (0) or down (!0).
    SYSCALL_GET_PEN_STATE,  // Gets the pen state.
    SYSCALL_SET_VISIBILITY, // Sets the turtle's visibility to hidden (0) or visible (!0).
    SYSCALL_GET_VISIBILITY, // Gets the turtle's visibility.
    SYSCALL_SET_PEN_COLOUR, // Sets the pen colour.
    SYSCALL_GET_PEN_COLOUR  // Gets the pen colour.
} Syscalls;

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

static inline void sys_exit(int32_t status)
{
    syscall1(SYSCALL_EXIT, status);
}

static inline void sys_home(void)
{
    syscall0(SYSCALL_HOME);
}

static inline void sys_ahead(float n)
{
    syscall1(SYSCALL_AHEAD, *(uint32_t*)&n);
}

static inline void sys_turn(float n)
{
    syscall1(SYSCALL_TURN, *(uint32_t*)&n);
}

static inline void sys_goto(float x, float y)
{
    syscall2(SYSCALL_GOTO, *(uint32_t*)&x, *(uint32_t*)&y);
}

static inline void sys_set_pen_state(bool isDown)
{
    syscall1(SYSCALL_SET_PEN_STATE, (uint32_t)isDown);
}

static inline bool sys_get_pen_state(void)
{
    return syscall0(SYSCALL_GET_PEN_STATE);
}

static inline void sys_set_visibility(bool isVisible)
{
    syscall1(SYSCALL_SET_VISIBILITY, (uint32_t)isVisible);
}

static inline bool sys_get_visibility(void)
{
    return syscall0(SYSCALL_GET_VISIBILITY);
}

static inline void sys_set_pen_colour(uint32_t rgba)
{
    syscall1(SYSCALL_SET_PEN_COLOUR, rgba);
}

static inline uint32_t sys_get_pen_colour(void)
{
    return syscall0(SYSCALL_GET_PEN_COLOUR);
}
