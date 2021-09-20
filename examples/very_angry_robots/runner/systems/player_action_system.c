#include "player_action_system.h"

#include "controller.h"
#include "entities.h"
#include "raylib.h"
#include "systems/event_system.h"
#include "tables/events.h"
#include "tables/positions.h"
#include "tables/velocities.h"

#define PLAYER_SPEED 2

static bool isEnabled = true;
static EntityId playerId = {-1};

static Vector2 GetPlayerMovement(void)
{
    // Find out what the player wants to do.
    float dx = 0.0f;
    float dy = 0.0f;
    const bool haveGamepad = IsGamepadAvailable(0);
    const bool usingKeyboard = GetController() == ctKEYBOARD || !haveGamepad;

    const bool isLeftDPadDown = haveGamepad && IsGamepadButtonDown(0, GAMEPAD_BUTTON_LEFT_FACE_LEFT);
    const bool isRightDPadDown = haveGamepad && IsGamepadButtonDown(0, GAMEPAD_BUTTON_LEFT_FACE_RIGHT);
    const bool isUpDPadDown = haveGamepad && IsGamepadButtonDown(0, GAMEPAD_BUTTON_LEFT_FACE_UP);
    const bool isDownDPadDown = haveGamepad && IsGamepadButtonDown(0, GAMEPAD_BUTTON_LEFT_FACE_DOWN);
    const bool anyGamepadButtonDown = isLeftDPadDown || isRightDPadDown || isUpDPadDown || isDownDPadDown;

    if (usingKeyboard || anyGamepadButtonDown)
    {
        // Move with the keyboard or the dpad.
        if (isRightDPadDown || IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT))
        {
            dx += 1.0f;
        }
        if (isLeftDPadDown || IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT))
        {
            dx -= 1.0f;
        }
        if (isUpDPadDown || IsKeyDown(KEY_W) || IsKeyDown(KEY_UP))
        {
            dy -= 1.0f;
        }
        if (isDownDPadDown || IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN))
        {
            dy += 1.0f;
        }

        // Keep the speed the same when moving diagonally.
        if (dx != 0.0f && dy != 0.0f)
        {
            const float sqrt2 = 0.7071067811865475f;
            dx *= sqrt2;
            dy *= sqrt2;
        }
    }
    else
    {
        // Move with the left stick.
        const float padX = GetGamepadAxisMovement(0, GAMEPAD_AXIS_LEFT_X);
        const float padY = GetGamepadAxisMovement(0, GAMEPAD_AXIS_LEFT_Y);
        const float angle = atan2f(padY, padX);
        float distance = hypotf(padX, padY);
        if (distance > 1.0f)
        {
            distance = 1.0f;
        }
        return (Vector2){cosf(angle) * distance, sinf(angle) * distance};
    }

    return (Vector2){dx, dy};
}

static Vector2 GetPlayerAim(void)
{
    const bool haveGamepad = IsGamepadAvailable(0);
    const bool usingMouseAim = GetController() == ctKEYBOARD || !haveGamepad;
    if (usingMouseAim)
    {
        // Aim with the mouse.
        const Vector2 mouseScreenPos = GetMousePosition();
        Camera2D camera = {.zoom = 1.0f}; // TODO: We need to find the camera from somewhere.
        const Vector2 mousePos = GetScreenToWorld2D(mouseScreenPos, camera);
        const Position* p = Positions.Get(playerId);
        const Vector2 playerPos = p->position;
        const Vector2 difference = Vector2Subtract(mousePos, playerPos);
        const float angle = atan2f(difference.y, difference.x);
        return (Vector2){cosf(angle), sinf(angle)};
    }
    else
    {
        // Aim using the right stick.
        const float padX = GetGamepadAxisMovement(0, GAMEPAD_AXIS_RIGHT_X);
        const float padY = GetGamepadAxisMovement(0, GAMEPAD_AXIS_RIGHT_Y);
        float distance = hypotf(padX, padY);

        // If we're in the dead-zone for aiming then just return the zero vector.,
        if (distance < 0.1f)
        {
            return (Vector2){0.0f, 0.0f};
        }

        const float angle = atan2f(padY, padX);
        return (Vector2){cosf(angle), sinf(angle)};
    }
}

static void UpdatePlayer(void)
{
    // How does the player want to move?
    const Vector2 movement = GetPlayerMovement();

    // Set its velocity.
    Velocity* v = Velocities.Get(playerId);
    v->velocity = Vector2Scale(movement, PLAYER_SPEED);

    // Get the player's aim.
    const Vector2 aim = GetPlayerAim();
    TraceLog(LOG_DEBUG, "Aim (%0.3f, %0.3f)", aim.x, aim.y);
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
