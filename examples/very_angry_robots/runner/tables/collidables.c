#include "collidables.h"

#include "entities.h"

#define WALL_SIZE 224
#define WALL_THICKNESS 2

#define DOOR_SIZE (WALL_SIZE - 80)
#define DOOR_THICKNESS 2

#define ROBOT_WIDTH 32
#define ROBOT_HEIGHT 32

#define PLAYER_WIDTH 32
#define PLAYER_HEIGHT 32

#define SHOT_WIDTH 2
#define SHOT_HEIGHT 2

static AABB geometries[] = {
        {.centre = {.x = 0.0f, .y = 0.0f}, .extents = {.x = ROBOT_WIDTH * 0.5f, .y = ROBOT_HEIGHT * 0.5f}},   // ctROBOT
        {.centre = {.x = 0.0f, .y = 0.0f}, .extents = {.x = PLAYER_WIDTH * 0.5f, .y = PLAYER_HEIGHT * 0.5f}}, // ctPLAYER
        {.centre = {.x = 0.0f, .y = 0.0f}, .extents = {.x = SHOT_WIDTH * 0.5f, .y = SHOT_HEIGHT * 0.5f}},     // ctSHOT
        {.centre = {.x = 0.0f, .y = 0.0f}, .extents = {.x = WALL_SIZE * 0.5f, .y = WALL_THICKNESS * 0.5f}},   // ctHWALL
        {.centre = {.x = 0.0f, .y = 0.0f}, .extents = {.x = WALL_THICKNESS * 0.5f, .y = WALL_SIZE * 0.5f}},   // ctVWALL
        {.centre = {.x = 0.0f, .y = 0.0f}, .extents = {.x = WALL_SIZE * 0.5f, .y = WALL_THICKNESS * 0.5f}},   // ctHDOOR
        {.centre = {.x = 0.0f, .y = 0.0f}, .extents = {.x = WALL_THICKNESS * 0.5f, .y = WALL_SIZE * 0.5f}},   // ctVDOOR
};
static struct Collidable collidables[MAX_ENTITIES];

Collidable* GetCollidable(EntityId id)
{
    return &collidables[id.id];
}

void SetCollidable(EntityId id, Collidable* collidable)
{
    collidables[id.id] = *collidable;
}

AABB GetCollidableGeometry(EntityId id)
{
    CollidableType type = collidables[id.id].type;
    return geometries[type];
}
