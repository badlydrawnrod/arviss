#include "raylib.h"
#include "raymath.h"

#include <stdbool.h>

typedef struct Turtle
{
    Vector2 lastPosition;
    Vector2 position;
    float angle;
    bool isVisible;
    bool isPenDown;
    Color penColour;
} Turtle;

void InitTurtle(Turtle* turtle)
{
    turtle->lastPosition = Vector2Zero();
    turtle->position = Vector2Zero();
    float angle = 0.0f;
    turtle->isVisible = true;
    turtle->isPenDown = true;
    turtle->penColour = RAYWHITE;
}

void Forward(Turtle* turtle, float distance)
{
    // Angle weirdness because we're using the points of the compass where North is zero degrees, East is 90 degrees, and so on.
    float angle = turtle->angle;
    Vector2 heading = (Vector2){sinf(angle), cosf(angle)};
    turtle->position = Vector2Add(turtle->position, Vector2Scale(heading, distance));
}

void Right(Turtle* turtle, float angle)
{
    turtle->angle += DEG2RAD * angle;
}

void CDrawLineV(Vector2 start, Vector2 end, Color colour)
{
    // I should really update the camera matrix to do this.
    DrawLine(start.x, -start.y, end.x, -end.y, colour);
}

int main(void)
{
    const int screenWidth = 1280;
    const int screenHeight = 720;

    InitWindow(screenWidth, screenHeight, "Turtles");
    SetTargetFPS(60);

    // Create the canvas that the turtles will draw onto.
    RenderTexture2D canvas = LoadRenderTexture(screenWidth, screenHeight);
    BeginTextureMode(canvas);
    ClearBackground(BLACK);
    EndTextureMode();

    int x = 0;
    int y = 0;

    Turtle turtles[8];
    for (int i = 0; i < sizeof(turtles) / sizeof(turtles[0]); i++)
    {
        InitTurtle(&turtles[i]);
        turtles[i].angle = i * DEG2RAD * 360.0f / (sizeof(turtles) / sizeof(turtles[0]));
    }

    // Create a camera whose origin is at the centre of the canvas.
    Vector2 origin = (Vector2){.x = canvas.texture.width / 2, .y = canvas.texture.height / 2};
    Camera2D camera = {0};
    camera.offset = origin;
    camera.zoom = 1.0f;

    float distance = 1.0f;

    while (!WindowShouldClose())
    {
        for (int i = 0; i < sizeof(turtles) / sizeof(turtles[0]); i++)
        {
            turtles[i].lastPosition = turtles[i].position;
        }
        for (int i = 0; i < sizeof(turtles) / sizeof(turtles[0]); i++)
        {
            Forward(&turtles[i], distance);
            Right(&turtles[i], 5.0f);
        }
        distance += 0.1f;
        BeginDrawing();
        ClearBackground(DARKBLUE);

        // If any of the turtles have moved then add the last line that they made to the canvas.
        BeginTextureMode(canvas);
        BeginMode2D(camera);
        for (int i = 0; i < sizeof(turtles) / sizeof(turtles[0]); i++)
        {
            if (turtles[i].isPenDown
                && (turtles[i].position.x != turtles[i].lastPosition.x || turtles[i].position.y != turtles[i].lastPosition.y))
            {
                DrawLineV(turtles[i].lastPosition, turtles[i].position, turtles[i].penColour);
            }
        }
        EndMode2D();
        EndTextureMode();

        // Draw the canvas that the turtles draw to.
        DrawTexture(canvas.texture, 0, 0, WHITE);

        // Draw the visible turtles.
        BeginMode2D(camera);
        for (int i = 0; i < sizeof(turtles) / sizeof(turtles[0]); i++)
        {
            // TODO: draw a turtle bitmap, suitably positioned and rotated.
        }
        EndMode2D();

        EndDrawing();
    }
    CloseWindow();

    return 0;
}
