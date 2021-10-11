#pragma once

typedef struct TimedTrigger
{
    GameTime time;
} TimedTrigger;

inline static void ClearTimedTrigger(TimedTrigger* trigger)
{
    trigger->time = 0.0;
}

inline static void SetTimedTrigger(TimedTrigger* trigger, GameTime when)
{
    trigger->time = when;
}

inline static bool PollTimedTrigger(TimedTrigger* trigger, GameTime now)
{
    if (trigger->time > 0.0 && now >= trigger->time)
    {
        trigger->time = 0.0;
        return true;
    }
    return false;
}
