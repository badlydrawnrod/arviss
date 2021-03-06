#pragma once

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
    SYSCALL_GET_PEN_COLOUR, // Gets the pen colour.
    SYSCALL_GET_POSITION,   // Gets the turtle's position.
    SYSCALL_GET_HEADING     // Gets the turtle's heading.
} Syscalls;
