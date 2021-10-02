#include "factory.h"

#include "entities.h"
#include "raylib.h"
#include "tables/aims.h"
#include "tables/collidables.h"
#include "tables/doors.h"
#include "tables/owners.h"
#include "tables/player_status.h"
#include "tables/positions.h"
#include "tables/rooms.h"
#include "tables/steps.h"
#include "tables/velocities.h"
#include "tables/walls.h"

#define WALL_SIZE 224
#define SHOT_SPEED 8
#define ROBOT_SHOT_SPEED 4

EntityId MakeShot(RoomId roomId, Vector2 position, Vector2 aim, EntityId owner)
{
    // TODO:  We also need to make sure that if the robot dies, and another entity comes into being, then it mustn't have the
    //  same entity id.
    EntityId id = (EntityId){Entities.Create()};
    Entities.Set(id, bmPosition | bmVelocity | bmDrawable | bmShot | bmCollidable | bmOwned | bmInRoom);
    Positions.Set(id, &(Position){.position = position});
    Velocities.Set(id, &(Velocity){.velocity = Vector2Scale(aim, SHOT_SPEED)});
    Collidables.Set(id, &(Collidable){.type = ctSHOT});
    Owners.Set(id, &(Owner){.ownerId = owner});
    Rooms.Set(id, &(Room){.roomId = roomId});
    return id;
}

EntityId MakeRobotShot(RoomId roomId, Vector2 position, Vector2 aim, EntityId owner)
{
    // TODO:  We also need to make sure that if the robot dies, and another entity comes into being, then it mustn't have the
    //  same entity id.
    EntityId id = (EntityId){Entities.Create()};
    Entities.Set(id, bmPosition | bmVelocity | bmDrawable | bmShot | bmCollidable | bmOwned | bmInRoom);
    Positions.Set(id, &(Position){.position = position});
    Velocities.Set(id, &(Velocity){.velocity = Vector2Scale(aim, ROBOT_SHOT_SPEED)});
    Collidables.Set(id, &(Collidable){.type = ctSHOT});
    Owners.Set(id, &(Owner){.ownerId = owner});
    Rooms.Set(id, &(Room){.roomId = roomId});
    return id;
}

EntityId MakeRobot(RoomId roomId, float x, float y)
{
    EntityId id = (EntityId){Entities.Create()};
    // No velocity to start because it isn't movable until the amnesty is over.
    Entities.Set(id, bmPosition | bmDrawable | bmRobot | bmCollidable | bmInRoom | bmStepped);
    Positions.Set(id, &(Position){.position = {x, y}});
    Velocities.Set(id, &(Velocity){.velocity = {1.0f, 0.0f}});
    Collidables.Set(id, &(Collidable){.type = ctROBOT});
    Rooms.Set(id, &(Room){.roomId = roomId});
    Steps.Set(id, &(Step){.rate = 32, .step = 0});
    return id;
}

EntityId MakePlayer(float x, float y)
{
    EntityId id = (EntityId){Entities.Create()};
    Entities.Set(id, bmPosition | bmVelocity | bmDrawable | bmPlayer | bmCollidable);
    Positions.Set(id, &(Position){.position = {x, y}});
    Velocities.Set(id, &(Velocity){.velocity = {0.0f, 0.0f}});
    PlayerStatuses.Set(id, &(PlayerStatus){.lives = 3, .score = 0});
    Collidables.Set(id, &(Collidable){.type = ctPLAYER});
    Aims.Set(id, &(Aim){.aim = {0.0f, 0.0f}});
    return id;
}

EntityId MakeWall(RoomId roomId, float x, float y, bool isVertical)
{
    EntityId id = (EntityId){Entities.Create()};
    Entities.Set(id, bmPosition | bmDrawable | bmWall | bmCollidable | bmInRoom);
    Positions.Set(id, &(Position){.position = {x, y}});
    Walls.Set(id, &(Wall){.vertical = isVertical});
    Collidables.Set(id, &(Collidable){.type = isVertical ? ctVWALL : ctHWALL});
    Rooms.Set(id, &(Room){.roomId = roomId});
    return id;
}

EntityId MakeWallFromGrid(RoomId roomId, int gridX, int gridY, bool isVertical)
{
    const float x = (float)gridX * WALL_SIZE + ((isVertical) ? 0 : WALL_SIZE / 2);
    const float y = (float)gridY * WALL_SIZE + ((isVertical) ? WALL_SIZE / 2 : 0);
    return MakeWall(roomId, x, y, isVertical);
}

EntityId MakeExit(RoomId roomId, float x, float y, bool isVertical, Entrance entrance)
{
    // An exit is an invisible door.
    EntityId id = (EntityId){Entities.Create()};
    Entities.Set(id, bmPosition | bmDoor | bmCollidable | bmInRoom);
    Positions.Set(id, &(Position){.position = {x, y}});
    Doors.Set(id, &(Door){.vertical = isVertical, .leadsTo = entrance});

    // It shares the same collision characteristics as a wall.
    Collidables.Set(id, &(Collidable){.type = isVertical ? ctVWALL : ctHWALL, .isTrigger = true});
    Rooms.Set(id, &(Room){.roomId = roomId});
    return id;
}

EntityId MakeExitFromGrid(RoomId roomId, int gridX, int gridY, bool isVertical, Entrance entrance)
{
    const float x = (float)gridX * WALL_SIZE + ((isVertical) ? 0 : WALL_SIZE / 2);
    const float y = (float)gridY * WALL_SIZE + ((isVertical) ? WALL_SIZE / 2 : 0);
    return MakeExit(roomId, x, y, isVertical, entrance);
}

EntityId MakeDoor(RoomId roomId, float x, float y, bool isVertical, Entrance entrance)
{
    EntityId id = (EntityId){Entities.Create()};
    Entities.Set(id, bmPosition | bmDrawable | bmDoor | bmCollidable | bmInRoom);
    Positions.Set(id, &(Position){.position = {x, y}});
    Doors.Set(id, &(Door){.vertical = isVertical, .leadsTo = entrance});
    Collidables.Set(id, &(Collidable){.type = isVertical ? ctVDOOR : ctHDOOR});
    Rooms.Set(id, &(Room){.roomId = roomId});
    return id;
}

EntityId MakeDoorFromGrid(RoomId roomId, int gridX, int gridY, bool isVertical, Entrance entrance)
{
    const float x = (float)gridX * WALL_SIZE + ((isVertical) ? 0 : WALL_SIZE / 2);
    const float y = (float)gridY * WALL_SIZE + ((isVertical) ? WALL_SIZE / 2 : 0);
    return MakeDoor(roomId, x, y, isVertical, entrance);
}
