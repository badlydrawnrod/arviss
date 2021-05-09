#pragma once

#include <stdbool.h>

// Note that these need to line up with the emulator itself otherwise it won't work.
#define ROMBASE 0
#define ROMSIZE 0x4000
#define RAMBASE (ROMBASE + ROMSIZE)
#define RAMSIZE 0x4000

#define MEMBASE ROMBASE
#define MEMSIZE (ROMSIZE + RAMSIZE)

#define IOBASE (RAMBASE + RAMSIZE)

#define TTY_STATUS IOBASE
#define TTY_DATA (TTY_STATUS + 1)
#define TTY_IS_WRITABLE 0x01

void _putchar(char c);
