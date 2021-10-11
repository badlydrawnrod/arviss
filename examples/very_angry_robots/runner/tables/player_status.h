#pragma once

#include "entities.h"
#include "raymath.h"

typedef struct PlayerStatus
{
    int lives;
    int score;
} PlayerStatus;

PlayerStatus* GetPlayerStatus(EntityId id);
void SetPlayerStatus(EntityId id, PlayerStatus* PlayerStatus);

static struct
{
    PlayerStatus* (*Get)(EntityId id);
    void (*Set)(EntityId id, PlayerStatus* PlayerStatus);
} PlayerStatuses = {.Get = GetPlayerStatus, .Set = SetPlayerStatus};
