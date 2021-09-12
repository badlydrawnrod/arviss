#include "player_status.h"

#include "entities.h"

static struct PlayerStatus playerStatuses[MAX_ENTITIES];

PlayerStatus* GetPlayerStatus(EntityId id)
{
    return &playerStatuses[id.id];
}

void SetPlayerStatus(EntityId id, PlayerStatus* playerStatus)
{
    playerStatuses[id.id] = *playerStatus;
}
