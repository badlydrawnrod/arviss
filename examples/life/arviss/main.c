#include "cell.h"

#include <stdbool.h>

#pragma clang diagnostic push
#pragma ide diagnostic ignored "EndlessLoop"
int main(void)
{
    for (;;)
    {
        bool wasAlive = GetState();
        int neighbours = CountNeighbours();
        SetState(wasAlive && neighbours == 2 || neighbours == 3);
    }
}
#pragma clang diagnostic pop
