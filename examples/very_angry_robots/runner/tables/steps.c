#include "steps.h"

#include "entities.h"

static struct Step steps[MAX_ENTITIES];

Step* GetStep(EntityId id)
{
    return &steps[id.id];
}

void SetStep(EntityId id, Step* step)
{
    steps[id.id] = *step;
}
