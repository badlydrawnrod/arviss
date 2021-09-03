#pragma once

#include <stdbool.h>
#include <stdint.h>

#define MAX_ENTITIES 256

typedef uint32_t ComponentBitmap;

typedef enum Component
{
    bmReap = 1,
    bmVelocity = bmReap << 1,
    bmPosition = bmVelocity << 1,
    bmPlayer = bmPosition << 1,
    bmRobot = bmPlayer << 1,
    bmWall = bmRobot << 1,
    bmDrawable = bmWall << 1,
    bmCollidable = bmDrawable << 1,
    bmShot = bmCollidable << 1,
    bmDoor = bmShot << 1
} Component;

typedef struct Entity
{
    ComponentBitmap bitmap;
} Entity;

typedef struct EntityId
{
    int id;
} EntityId;

void ResetEntities(void);                      // Remove all entities.
int CountEntity(void);                         // The current number of active entities.
int MaxCountEntity(void);                      // What's the largest number of entities that have been active?
int CreateEntity(void);                        // Creates a new entity.
void DestroyEntity(EntityId id);               // Destroy this entity by removing all of its components.
bool IsEntity(EntityId id, Component mask);    // True if this entity exactly matches a component.
bool AnyOfEntity(EntityId id, Component mask); // True if this entity matches any of the components.
void ClearEntity(EntityId id, Component mask); // Removes one or more components from an entity.
void SetEntity(EntityId id, Component mask);   // Adds one of more components to an entity.

static struct
{
    void (*Reset)(void);
    int (*Count)(void);
    int (*MaxCount)(void);
    int (*Create)(void);
    void (*Destroy)(EntityId id);
    bool (*Is)(EntityId id, Component mask);
    bool (*AnyOf)(EntityId id, Component mask);
    void (*Clear)(EntityId id, Component mask);
    void (*Set)(EntityId id, Component mask);
} Entities = {.Reset = ResetEntities,
              .Count = CountEntity,
              .MaxCount = MaxCountEntity,
              .Create = CreateEntity,
              .Destroy = DestroyEntity,
              .Is = IsEntity,
              .AnyOf = AnyOfEntity,
              .Clear = ClearEntity,
              .Set = SetEntity};
