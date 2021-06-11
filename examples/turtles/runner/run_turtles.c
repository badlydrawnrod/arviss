#include "raylib.h"
#include "raymath.h"

#include <float.h>
#include <math.h>
#include <stdbool.h>

#define TURTLE_SCALE 12.0f
#define NUM_TURTLES 10
#define MAX_LINES 12
#define MAX_TURN (5.0f * DEG2RAD)
#define MAX_SPEED 1.0f

typedef struct Turtle
{
    Vector2 lastPosition;
    Vector2 position;
    float angle;
    bool isVisible;
    bool isPenDown;
    Color penColour;
    float aheadRemaining;
    float turnRemaining;
} Turtle;

// Types of draw command.
typedef enum
{
    END,  // Indicates the last command.
    MOVE, // Move to a given position.
    LINE  // Draw a line from the current position to the given position. If there is no current position, start from the origin.
} CommandType;

// A draw command.
typedef struct
{
    CommandType type;
    Vector2 pos;
} DrawCommand;

// Turtle appearance.
static const DrawCommand turtleShape[MAX_LINES] = {{MOVE, {-1, 1}},   {LINE, {0, -1}}, {LINE, {1, 1}},
                                                   {LINE, {0, 0.5f}}, {LINE, {-1, 1}}, {END, {0, 0}}};

static void InitTurtle(Turtle* turtle)
{
    turtle->lastPosition = Vector2Zero();
    turtle->position = Vector2Zero();
    turtle->angle = 0.0f;
    turtle->isVisible = true;
    turtle->isPenDown = true;
    turtle->penColour = RAYWHITE;
    turtle->aheadRemaining = 0.0f;
    turtle->turnRemaining = 0.0f;
}

static void SetAhead(Turtle* turtle, float distance)
{
    turtle->aheadRemaining = distance;
    turtle->turnRemaining = 0.0f;
}

static void SetTurn(Turtle* turtle, float angle)
{
    turtle->turnRemaining = angle * DEG2RAD;
    turtle->aheadRemaining = 0.0f;
}

static float NormalizeAngle(float angle)
{
    // Obviously there's a nicer way of doing this.
    while (angle < -PI)
    {
        angle += 2 * PI;
    }
    while (angle >= PI)
    {
        angle -= 2 * PI;
    }
    return angle;
}

static void Update(Turtle* turtle)
{
    turtle->lastPosition = turtle->position;

    if (fabsf(turtle->aheadRemaining) >= FLT_EPSILON)
    {
        float magDistance = fminf(MAX_SPEED, fabsf(turtle->aheadRemaining));
        float distance = copysignf(magDistance, turtle->aheadRemaining);

        float angle = turtle->angle;
        Vector2 heading = (Vector2){sinf(angle), cosf(angle)};
        turtle->position = Vector2Add(turtle->position, Vector2Scale(heading, distance));
        turtle->aheadRemaining -= distance;
    }
    else if (fabsf(turtle->turnRemaining) >= FLT_EPSILON)
    {
        float magTurn = fminf(MAX_TURN, fabsf(turtle->turnRemaining));
        float turn = copysignf(magTurn, turtle->turnRemaining);
        turtle->angle += turn;
        turtle->angle = NormalizeAngle(turtle->angle);
        turtle->turnRemaining -= turn;
    }
}

static void Goto(Turtle* turtle, float x, float y)
{
    turtle->position.x = x;
    turtle->position.y = y;
}

static void DrawTurtle(Vector2 pos, float heading, Color colour)
{
    Vector2 points[MAX_LINES];

    // Default to starting at the origin.
    pos.y = -pos.y;
    Vector2 here = Vector2Add(Vector2Scale(Vector2Rotate((Vector2){0, 0}, heading), TURTLE_SCALE), pos);

    int numPoints = 0;
    const DrawCommand* commands = turtleShape;
    for (int i = 0; commands[i].type != END; i++)
    {
        const Vector2 coord = Vector2Add(Vector2Scale(Vector2Rotate(commands[i].pos, heading), TURTLE_SCALE), pos);
        if (commands[i].type == LINE)
        {
            if (numPoints == 0)
            {
                points[0] = here;
                ++numPoints;
            }
            points[numPoints] = coord;
            ++numPoints;
        }
        else if (commands[i].type == MOVE)
        {
            if (numPoints > 0)
            {
                DrawLineStrip(points, numPoints, colour);
                numPoints = 0;
            }
        }
        here = coord;
    }
    if (numPoints > 0)
    {
        DrawLineStrip(points, numPoints, colour);
    }
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

    Turtle turtles[NUM_TURTLES];
    for (int i = 0; i < sizeof(turtles) / sizeof(turtles[0]); i++)
    {
        InitTurtle(&turtles[i]);
        turtles[i].angle = i * DEG2RAD * 360.0f / (sizeof(turtles) / sizeof(turtles[0]));
    }

    // Create a camera whose origin is at the centre of the canvas.
    Vector2 origin = (Vector2){.x = (float)(canvas.texture.width / 2), .y = (float)(canvas.texture.height / 2)};
    Camera2D camera = {0};
    camera.offset = origin;
    camera.zoom = 1.0f;

    float distance = 0.0f;

    bool moving = true;
    for (int i = 0; i < sizeof(turtles) / sizeof(turtles[0]); i++)
    {
        SetAhead(&turtles[i], 250.0f);
    }

    while (!WindowShouldClose())
    {
        for (int i = 0; i < sizeof(turtles) / sizeof(turtles[0]); i++)
        {
            Update(&turtles[i]);
        }
        if (moving && turtles[0].aheadRemaining == 0.0f)
        {
            moving = false;
            for (int i = 0; i < sizeof(turtles) / sizeof(turtles[0]); i++)
            {
                SetTurn(&turtles[i], 90.0f);
            }
        }
        else if (!moving && turtles[0].turnRemaining == 0.0f)
        {
            moving = true;
            for (int i = 0; i < sizeof(turtles) / sizeof(turtles[0]); i++)
            {
                SetAhead(&turtles[i], 250.0f);
            }
        }
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

        // Draw the visible turtles above the canvas.
        BeginMode2D(camera);
        for (int i = 0; i < sizeof(turtles) / sizeof(turtles[0]); i++)
        {
            if (turtles[i].isVisible)
            {
                DrawTurtle(turtles[i].position, turtles[i].angle * RAD2DEG, turtles[i].penColour);
            }
        }
        EndMode2D();

        EndDrawing();
    }
    CloseWindow();

    return 0;
}
