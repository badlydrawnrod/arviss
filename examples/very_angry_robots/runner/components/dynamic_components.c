#include "dynamic_components.h"

#include "entities.h"

static struct DynamicComponent dynamicComponents[MAX_ENTITIES];

DynamicComponent* GetDynamicComponent(int id)
{
    return &dynamicComponents[id];
}

void SetDynamicComponent(int id, DynamicComponent* dynamicComponent)
{
    dynamicComponents[id] = *dynamicComponent;
}

Vector2 GetDynamicComponentPosition(int id)
{
    return dynamicComponents[id].position;
}
