#include "robot_action_system.h"

#include "factory.h"
#include "systems/event_system.h"
#include "tables/events.h"
#include "tables/positions.h"

static bool isEnabled = true;
static EntityId playerId = {.id = -1};

static void HandleEvents(int first, int last)
{
    for (int i = first; i != last; i++)
    {
        const Event* e = Events.Get((EventId){.id = i});
        if (e->type == etDOOR)
        {
            const DoorEvent* de = &e->door;
            isEnabled = de->type == deENTER;
        }
    }
}

void ResetRobotActions(void)
{
    isEnabled = true;
    EventSystem.Register(HandleEvents);
}

void UpdateRobotActions(void)
{
    if (!isEnabled)
    {
        return;
    }

    // Cache the player id.
    if (playerId.id == -1)
    {
        for (int i = 0, numEntities = Entities.MaxCount(); i < numEntities; i++)
        {
            if (Entities.Is((EntityId){i}, bmPlayer))
            {
                playerId.id = i;
                break;
            }
        }
    }

    for (int i = 0, numEntities = Entities.MaxCount(); i < numEntities; i++)
    {
        EntityId id = {i};
        if (Entities.Is(id, bmRobot | bmPosition | bmVelocity))
        {
            Position* p = Positions.Get(id);
            // TODO: factor this out to "fire", presumably at the player.
            if (((int)p->position.x % 47) == 0)
            {
                Vector2 playerPos = Positions.GetPosition(playerId);
                Vector2 robotPos = p->position;
                float angle = atan2f(playerPos.y - robotPos.y, playerPos.x - robotPos.x);
                Vector2 aim = {cosf(angle), sinf(angle)};
                MakeRobotShot(p->position, aim, id);
            }
        }
    }
}
