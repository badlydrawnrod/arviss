#include "syscalls.h"

#include <stdbool.h>

int main(void)
{
    sys_set_pen_state(true);
    sys_set_visibility(true);
    sys_set_pen_colour(0xffffffff);
    for (int i = 0; i < 4; i++)
    {
        sys_ahead(250);
        sys_turn(90);
        sys_set_pen_colour(sys_get_pen_colour() == 0xffffffff ? 0x00ffffff : 0xffffffff);
        sys_set_visibility(!sys_get_visibility());
    }
    sys_turn(45);

    sys_set_visibility(false);
    sys_set_pen_colour(0x00ff00ff);
    for (int i = 0; i < 4; i++)
    {
        sys_set_pen_state(!sys_get_pen_state());
        sys_ahead(250);
        sys_turn(90);
    }
    sys_turn(45);

    sys_set_visibility(true);
    for (int i = 0; i < 4; i++)
    {
        sys_set_pen_state(!sys_get_pen_state());
        sys_ahead(250);
        sys_turn(90);
    }
    sys_turn(45);

    // Go home, become invisible, then exit.
    sys_home();
    sys_set_visibility(false);

    return 0;
}
