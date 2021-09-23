#include "player_controls.h"

#include "entities.h"

static struct PlayerControl playerControls[MAX_ENTITIES];

PlayerControl* GetPlayerControl(EntityId id)
{
    return &playerControls[id.id];
}

void SetPlayerControl(EntityId id, PlayerControl* playerControl)
{
    playerControls[id.id] = *playerControl;
}
