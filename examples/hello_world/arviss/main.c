#include "printf.h"

int main(void)
{
    for (int i = 0; i < 10000000; i++)
    {
        printf("Hello, world!\n");
    }
    return 0;
}
