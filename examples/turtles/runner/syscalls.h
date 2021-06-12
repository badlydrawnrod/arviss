#pragma once

typedef enum Syscalls
{
    SYSCALL_EXIT,  // The turtle's program has finished.
    SYSCALL_HOME,  // Tells the turtle to return to the origin, facing up.
    SYSCALL_AHEAD, // Tells the turtle to move ahead (or behind if negative) by the given amount.
    SYSCALL_TURN,  // Tells the turtle to turn right (or left if negative) by the given amount.
    SYSCALL_GOTO   // Tells the turtle to teleport to the given location.
} Syscalls;