#include "game_status_system.h"

#include "components/events.h"
#include "raylib.h"

static int lives;

void ResetGameStatusSystem(void)
{
    lives = 3;
}

void UpdateGameStatusSystem(void)
{
    bool died = false;

    for (int i = 0, numEvents = Events.Count(); i < numEvents; i++)
    {
        const Event* e = Events.Get((EventId){.id = i});
        if (e->type != etPLAYER)
        {
            continue;
        }

        const PlayerEvent* pe = &e->player;
        if (!died && pe->type == peDIED)
        {
            died = true;
            --lives;
            TraceLog(LOG_INFO, "Player died. Lives reduced to %d", lives);
        }
    }
}
