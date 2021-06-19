#include "syscalls.h"

#include <stdbool.h>

int main(void)
{
    // Draw a white square, then turn right by 45 degrees.
    sys_set_pen_state(true);
    sys_set_visibility(true);
    sys_set_pen_colour(0xffffffff);
    for (int i = 0; i < 4; i++)
    {
        sys_ahead(250);
        sys_turn(90);
    }
    sys_turn(45);

    // Hide the turtle, draw a cyan square, then turn right by 45 degrees.
    sys_set_visibility(false);
    sys_set_pen_colour(0x00ffffff);
    for (int i = 0; i < 4; i++)
    {
        sys_ahead(250);
        sys_turn(90);
    }
    sys_turn(45);

    // Show the turtle, raise the pen, move in a square, then turn right by 45 degrees.
    sys_set_visibility(true);
    sys_set_pen_colour(0x00ff00ff);
    sys_set_pen_state(false);
    for (int i = 0; i < 4; i++)
    {
        sys_ahead(250);
        sys_turn(90);
    }
    sys_turn(45);

    // Go home, become invisible, then exit.
    sys_home();
    sys_set_visibility(false);

    return 0;
}
