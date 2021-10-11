#pragma once

#include "entities.h"

typedef struct Step
{
    int step;
    int rate;
} Step;

Step* GetStep(EntityId id);
void SetStep(EntityId id, Step* step);

static struct
{
    Step* (*Get)(EntityId id);
    void (*Set)(EntityId id, Step* step);
} Steps = {.Get = GetStep, .Set = SetStep};
