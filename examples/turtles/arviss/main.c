#include "syscalls.h"

#include <stdbool.h>

int main(void)
{
    while (true)
    {
        sys_ahead(250);
        sys_turn(90);
    }
}
