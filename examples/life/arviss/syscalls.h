#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef enum Syscalls
{
    SYSCALL_COUNT,     // Count neighbours.
    SYSCALL_GET_STATE, // Gets the state of this cell.
    SYSCALL_SET_STATE  // Sets the state of this cell.
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
