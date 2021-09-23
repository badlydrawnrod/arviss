#pragma once

#include "entities.h"
#include "raymath.h"

typedef struct PlayerControl
{
    Vector2 movement;
    Vector2 aim;
    bool fire;
} PlayerControl;

PlayerControl* GetPlayerControl(EntityId id);
void SetPlayerControl(EntityId id, PlayerControl* playerControl);

static struct
{
    PlayerControl* (*Get)(EntityId id);
    void (*Set)(EntityId id, PlayerControl* playerControl);
} PlayerControls = {.Get = GetPlayerControl, .Set = SetPlayerControl};
