#include "syscalls.h"

#include <stdbool.h>
#include <stddef.h>

#pragma clang diagnostic push
#pragma ide diagnostic ignored "EndlessLoop"
int main(void)
{
    for (;;)
    {
        // Where am I?
        RkVector myPosition;
        RkGetMyPosition(&myPosition);

        // Where's the player?
        RkVector playerPosition;
        RkGetPlayerPosition(&playerPosition);

        const float probeDistance = 16.0f;

        // Is there anything that I might hit between me and the player?
        if (!RkRaycastTowards(&playerPosition, probeDistance))
        {
            // Move in the direction of the player.
            RkMoveTowards(&playerPosition);
        }
        else if (!RkRaycastTowards(&(RkVector){.x = playerPosition.x, .y = myPosition.y}, probeDistance))
        {
            // Move horizontally towards the player.
            RkMoveTowards(&(RkVector){.x = playerPosition.x, .y = myPosition.y});
        }
        else if (!RkRaycastTowards(&(RkVector){.x = myPosition.x, .y = playerPosition.y}, probeDistance))
        {
            // Move vertically towards the player.
            RkMoveTowards(&(RkVector){.x = myPosition.x, .y = playerPosition.y});
        }
        else
        {
            // We can't move without hitting anything, so stop.
            RkStop();
        }

        // Fire at the player.
        if (((int)myPosition.x % 47) == 0)
        {
            RkFireAt(&playerPosition);
        }

        RkYield();
    }
}
#pragma clang diagnostic pop
