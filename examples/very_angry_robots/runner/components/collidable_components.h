#pragma once

typedef enum CollidableType
{
    ctROBOT,
    ctPLAYER,
    ctHWALL,
    ctVWALL,
    ctHDOOR,
    ctVDOOR
} CollidableType;

typedef struct CollidableComponent
{
    CollidableType type;
} CollidableComponent;

CollidableComponent* GetCollidableComponent(int id);
void SetCollidableComponent(int id, CollidableComponent* collidableComponent);

static struct
{
    CollidableComponent* (*Get)(int id);
    void (*Set)(int id, CollidableComponent* collidableComponent);
} CollidableComponents = {.Get = GetCollidableComponent, .Set = SetCollidableComponent};
