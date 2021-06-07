#include "putchar.h"

#include <stdbool.h>

static inline bool tty_is_writable()
{
    return (*((char*)TTY_STATUS)) & TTY_IS_WRITABLE;
}

static inline void tty_putc(char c)
{
    while (!tty_is_writable())
    {
    }
    *((char*)TTY_DATA) = c;
}

void _putchar(char c)
{
    tty_putc(c);
}
