#include "robot.h"

#include <stdbool.h>
#include <stddef.h>

#pragma clang diagnostic push
#pragma ide diagnostic ignored "EndlessLoop"
int main(void)
{
    for (;;)
    {
        // Where am I?
        Vector myPosition;
        GetMyPosition(&myPosition);

        // Where's the player?
        Vector playerPosition;
        GetPlayerPosition(&playerPosition);

        const float probeDistance = 16.0f;

        // Is there anything that I might hit between me and the player?
        if (!RaycastTowards(&playerPosition, probeDistance))
        {
            // Move in the direction of the player.
            MoveTowards(&playerPosition);
        }
        else if (!RaycastTowards(&(Vector){.x = playerPosition.x, .y = myPosition.y}, probeDistance))
        {
            // Move horizontally towards the player.
            MoveTowards(&(Vector){.x = playerPosition.x, .y = myPosition.y});
        }
        else if (!RaycastTowards(&(Vector){.x = myPosition.x, .y = playerPosition.y}, probeDistance))
        {
            // Move vertically towards the player.
            MoveTowards(&(Vector){.x = myPosition.x, .y = playerPosition.y});
        }
        else
        {
            // We can't move without hitting anything, so stop.
            Stop();
        }

        // Fire at the player.
        if (((int)myPosition.x % 11) == 0)
        {
            FireAt(&playerPosition);
        }

        Yield();
    }
}
#pragma clang diagnostic pop
