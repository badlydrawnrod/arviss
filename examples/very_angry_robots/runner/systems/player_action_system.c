#include "player_action_system.h"

#include "entities.h"
#include "raylib.h"
#include "systems/event_system.h"
#include "tables/aims.h"
#include "tables/collidables.h"
#include "tables/events.h"
#include "tables/owners.h"
#include "tables/player_controls.h"
#include "tables/player_status.h"
#include "tables/positions.h"
#include "tables/velocities.h"

#define PLAYER_SPEED 2
#define SHOT_SPEED 8

static bool isEnabled = true;
static EntityId playerId = {-1};

static inline Vector2 GetPlayerMovement(void)
{
    PlayerControl* control = PlayerControls.Get(playerId);
    return control->movement;
}

static inline Vector2 GetPlayerAim(void)
{
    PlayerControl* control = PlayerControls.Get(playerId);
    return control->aim;
}

static inline bool GetPlayerFire()
{
    PlayerControl* control = PlayerControls.Get(playerId);
    const bool shouldFire = control->fire;
    control->fire = false;
    return shouldFire;
}

static EntityId MakeShot(Vector2 position, Vector2 aim, EntityId owner)
{
    // TODO: if an entity has an owner, e.g., a shot was fired by a robot, then we need to make sure that the shot doesn't collide
    //  with the robot that fired it. We also need to make sure that if the robot dies, and another entity comes into being, then
    //  it mustn't have the same entity id.
    EntityId id = (EntityId){Entities.Create()};
    Entities.Set(id, bmPosition | bmVelocity | bmDrawable | bmShot | bmCollidable | bmOwned);
    Positions.Set(id, &(Position){.position = position});
    Velocities.Set(id, &(Velocity){.velocity = Vector2Scale(aim, SHOT_SPEED)});
    Collidables.Set(id, &(Collidable){.type = ctSHOT});
    Owners.Set(id, &(Owner){.ownerId = owner});
    return id;
}

static void UpdatePlayer(void)
{
    // Find out how the player wants to move and set its velocity.
    const Vector2 movement = GetPlayerMovement();
    Velocity* v = Velocities.Get(playerId);
    v->velocity = Vector2Scale(movement, PLAYER_SPEED);

    // Find out where the player wants to aim and set its direction.
    const Vector2 aim = GetPlayerAim();
    Aim* a = Aims.Get(playerId);
    a->aim = aim;

    // Find out if the player wants to fire a shot.
    bool isFiring = GetPlayerFire();
    if (isFiring)
    {
        TraceLog(LOG_DEBUG, "TODO: player fires a shot in the direction that they're aiming");
        Position* p = Positions.Get(playerId);
        MakeShot(p->position, aim, playerId);
    }
}

static void HandleEvents(int first, int last)
{
    for (int i = first; i != last; i++)
    {
        const Event* e = Events.Get((EventId){.id = i});
        if (e->type == etDOOR)
        {
            const DoorEvent* de = &e->door;
            isEnabled = de->type == deENTER;
        }
    }
}

void ResetPlayerActions(void)
{
    isEnabled = true;
    EventSystem.Register(HandleEvents);
}

void UpdatePlayerActions(void)
{
    // Cache the player id.
    if (playerId.id == -1)
    {
        for (int i = 0, numEntities = Entities.MaxCount(); i < numEntities; i++)
        {
            if (Entities.Is((EntityId){i}, bmPlayer))
            {
                playerId.id = i;
                break;
            }
        }
    }

    if (!isEnabled)
    {
        return;
    }

    UpdatePlayer();
}
