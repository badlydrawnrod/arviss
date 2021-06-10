#include "raylib.h"
#include "raymath.h"

#include <stdbool.h>

#define TURTLE_SCALE 12.0f
#define NUM_TURTLES 10
#define MAX_LINES 12

typedef struct Turtle
{
    Vector2 lastPosition;
    Vector2 position;
    float angle;
    bool isVisible;
    bool isPenDown;
    Color penColour;
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
    float angle = 0.0f;
    turtle->isVisible = true;
    turtle->isPenDown = true;
    turtle->penColour = RAYWHITE;
}

static void Forward(Turtle* turtle, float distance)
{
    // Angle weirdness because we're using the points of the compass where North is zero degrees, East is 90 degrees, and so on.
    float angle = turtle->angle;
    Vector2 heading = (Vector2){sinf(angle), cosf(angle)};
    turtle->position = Vector2Add(turtle->position, Vector2Scale(heading, distance));
}

static void Right(Turtle* turtle, float angle)
{
    turtle->angle += DEG2RAD * angle;
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

    while (!WindowShouldClose())
    {
        for (int i = 0; i < sizeof(turtles) / sizeof(turtles[0]); i++)
        {
            turtles[i].lastPosition = turtles[i].position;
        }
        for (int i = 0; i < sizeof(turtles) / sizeof(turtles[0]); i++)
        {
            Forward(&turtles[i], distance);
            Right(&turtles[i], 27.5f);
        }
        distance += 2.0f;
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
