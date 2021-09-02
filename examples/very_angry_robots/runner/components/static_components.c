#include "static_components.h"

#include "entities.h"

static struct StaticComponent staticComponents[MAX_ENTITIES];

StaticComponent* GetStaticComponent(int id)
{
    return &staticComponents[id];
}

void SetStaticComponent(int id, StaticComponent* staticComponent)
{
    staticComponents[id] = *staticComponent;
}

Vector2 GetStaticComponentPosition(int id)
{
    return staticComponents[id].position;
}
