#pragma once

#include <stdbool.h>
#include <stdint.h>

#define MAX_ENTITIES 256

typedef uint32_t Bitmap;

typedef enum Component
{
    bmVelocity = 1,
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
    Bitmap bitmap;
} Entity;

typedef struct EntityId
{
    int id;
} EntityId;

void ResetEntities(void);
int CountEntities(void);
int CreateEntity(void);
void DestroyEntity(EntityId id);
bool IsEntity(EntityId id, Component mask);
bool AnyOfEntity(EntityId id, Component mask);
void ClearEntity(EntityId id, Component mask);
void SetEntity(EntityId id, Component mask);

static struct
{
    void (*Reset)(void);
    int (*Count)(void);
    int (*Create)(void);
    void (*Destroy)(EntityId id);
    bool (*Is)(EntityId id, Component mask);
    bool (*AnyOf)(EntityId id, Component mask);
    void (*Clear)(EntityId id, Component mask);
    void (*Set)(EntityId id, Component mask);
} Entities = {.Reset = ResetEntities,
              .Count = CountEntities,
              .Create = CreateEntity,
              .Destroy = DestroyEntity,
              .Is = IsEntity,
              .AnyOf = AnyOfEntity,
              .Clear = ClearEntity,
              .Set = SetEntity};
