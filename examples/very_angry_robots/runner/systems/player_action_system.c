#include "player_action_system.h"

#include "components/velocities.h"
#include "contoller.h"
#include "entities.h"
#include "raylib.h"

#define PLAYER_SPEED 2

static EntityId playerId = {-1};

static void UpdatePlayer(void)
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
        const float padX = GetGamepadAxisMovement(0, GAMEPAD_AXIS_LEFT_X);
        const float padY = GetGamepadAxisMovement(0, GAMEPAD_AXIS_LEFT_Y);
        const float angle = atan2f(padY, padX);
        float distance = hypotf(padX, padY);
        if (distance > 1.0f)
        {
            distance = 1.0f;
        }
        dx = cosf(angle) * distance;
        dy = sinf(angle) * distance;
    }

    // Set its velocity.
    Velocity* v = Velocities.Get(playerId);
    v->velocity.x = dx * PLAYER_SPEED;
    v->velocity.y = dy * PLAYER_SPEED;
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

    UpdatePlayer();
}