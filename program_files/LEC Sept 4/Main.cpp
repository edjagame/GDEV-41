#include <raylib.h>
#include <raymath.h>
#include <vector>
#include <string>
#include <fstream>

const int WINDOW_WIDTH = 800;
const int WINDOW_HEIGHT = 600;
const int FPS = 60;
const float TIMESTEP = 1.0f / FPS;

struct Ball {
    Vector2 position;
    Vector2 acceleration;
    Vector2 velocity;

    float mass;
    float inverse_mass;
    float friction;

    float radius;
    Color color;
};



int main() {
    InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Lecture");

    Ball b;
    b.position = {WINDOW_WIDTH / 2, WINDOW_HEIGHT / 2};
    b.acceleration = Vector2Zero();
    b.velocity = Vector2Zero();

    b.mass = 1.0f;
    b.inverse_mass = 1.0f / b.mass;
    b.friction = 0.95f;

    b.radius = 50;
    b.color = {100, 200, 150, 255};

    float deltaTime = 0.0f;
    float force = 500.0f;

    SetTargetFPS(FPS);
    float accumulator = 0.0f;


    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(BLACK);

        DrawCircleV(b.position, b.radius, b.color);

        deltaTime = GetFrameTime();

        Vector2 forces = Vector2Zero();

        if(IsKeyDown(KEY_W)) {
            forces = Vector2Add(forces, {0, -force});
        }
        if(IsKeyDown(KEY_S)) {
            forces = Vector2Add(forces, {0, force});
        }
        if(IsKeyDown(KEY_A)) {
            forces = Vector2Add(forces, {-force, 0});
        }
        if(IsKeyDown(KEY_D)) {
            forces = Vector2Add(forces, {force, 0});
        }

        b.acceleration = Vector2Scale(forces, b.inverse_mass);

        accumulator += deltaTime;

        while(accumulator >= TIMESTEP) {
            b.velocity = Vector2Add(b.velocity, Vector2Scale(b.acceleration, TIMESTEP));
            b.velocity = Vector2Scale(b.velocity, b.friction);
            b.position = Vector2Add(b.position, Vector2Scale(b.velocity, TIMESTEP));
            accumulator -= TIMESTEP;
        }

        EndDrawing();
    }

    CloseWindow();
    return 0;
}