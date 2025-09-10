#include <raylib.h>
#include <raymath.h>
#include <vector>

const int WINDOW_WIDTH = 800;
const int WINDOW_HEIGHT = 600;
const float FPS = 60;
const float TIMESTEP = 1 / FPS; // Sets the timestep to 1 / FPS. But timestep can be any very small value.
const float FRICTION = 0.5;


struct Ball {
    Vector2 position;
    float radius;
    Color color;

    float mass;
    float inverse_mass; // A variable for 1 / mass. Used in the calculation for acceleration = sum of forces / mass
    Vector2 acceleration;
    Vector2 velocity;
};

int main() {
    std::vector<Ball> balls(30);

    for (int i = 0; i < 6; ++i) {
        for (int j = 0; j < 5; j++) {
            balls[i * 5 + j].position = {60.0f + 120 * i, 60.0f + 120 * j};
            balls[i * 5 + j].radius = 50.0f;
            balls[i * 5 + j].color = (i * 5 + j == 0) ? RED : BLUE;
            balls[i * 5 + j].mass = 1.0f;
            balls[i * 5 + j].inverse_mass = 1.0f / balls[i * 5 + j].mass;
            balls[i * 5 + j].acceleration = Vector2Zero();
            balls[i * 5 + j].velocity = Vector2Zero();
        }
    }

    Ball* playerBall = &balls[0];
    

    InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Physics Demo");

    SetTargetFPS(FPS);

    float accumulator = 0;
    float elasticity = 1.0;

    while (!WindowShouldClose()) {
        float delta_time = GetFrameTime();
        Vector2 forces = Vector2Zero(); // every frame set the forces to a 0 vector

        // Adds forces with the magnitude of 100 in the direction given by WASD inputs
        if(IsKeyDown(KEY_W)) {
            forces = Vector2Add(forces, {0, -100});
        }
        if(IsKeyDown(KEY_A)) {
            forces = Vector2Add(forces, {-100, 0});
        }
        if(IsKeyDown(KEY_S)) {
            forces = Vector2Add(forces, {0, 100});
        }
        if(IsKeyDown(KEY_D)) {
            forces = Vector2Add(forces, {100, 0});
        }
        if(IsKeyPressed(KEY_SPACE)) {
            elasticity = (elasticity == 1.0) ? 0.0 : 1.0;
        }

        for(Ball &ball : balls) {
            // Does Vector - Scalar multiplication with the sum of all forces and the inverse mass of the ball
            playerBall->acceleration = Vector2Scale(forces, playerBall->inverse_mass);

            // Physics step
            accumulator += delta_time;
            while(accumulator >= TIMESTEP) {
                // ------ SEMI-IMPLICIT EULER INTEGRATION -------
                // Computes for velocity using v(t + dt) = v(t) + (a(t) * dt)
                ball.velocity = Vector2Add(ball.velocity, Vector2Scale(ball.acceleration, TIMESTEP));
                ball.velocity = Vector2Subtract(ball.velocity, Vector2Scale(ball.velocity, FRICTION * ball.inverse_mass * TIMESTEP));
                // Computes for change in position using x(t + dt) = x(t) + (v(t + dt) * dt)
                ball.position = Vector2Add(ball.position, Vector2Scale(ball.velocity, TIMESTEP));

                
                //Compute Collision with other balls
                for(Ball &otherBall : balls){

                    if (&otherBall == &ball) continue;
                    if (Vector2Distance(ball.position, otherBall.position) >= ball.radius + otherBall.radius) continue;

                    Vector2 normal = Vector2Subtract(otherBall.position, ball.position);
                    Vector2 relativeVelocity = Vector2Subtract(otherBall.velocity, ball.velocity);
                    float result = Vector2DotProduct(relativeVelocity, normal);
                    if(result < 0) {
                        Vector2 n = Vector2Normalize(normal);
                        float r = Vector2DotProduct(relativeVelocity, n);
                        float impulse = (1 + elasticity) * r / ((Vector2Length(n)) * (ball.inverse_mass + otherBall.inverse_mass));
                        ball.velocity = Vector2Add(ball.velocity, Vector2Scale(n, impulse * ball.inverse_mass));
                        otherBall.velocity = Vector2Subtract(otherBall.velocity, Vector2Scale(n, impulse * otherBall.inverse_mass));
                    }
                }



                // Negates the velocity at x and y if the object hits a wall. (Basic Collision Detection)
                if(ball.position.x + ball.radius >= WINDOW_WIDTH || ball.position.x - ball.radius <= 0) {
                    ball.velocity.x *= -1;
                }
                if(ball.position.y + ball.radius >= WINDOW_HEIGHT || ball.position.y - ball.radius <= 0) {
                    ball.velocity.y *= -1;
                }

                accumulator -= TIMESTEP;
            }
        }
        BeginDrawing();
        ClearBackground(WHITE);
        for(Ball &ball : balls) {
            DrawCircleV(ball.position, ball.radius, ball.color);
        }
        EndDrawing();
    }
    CloseWindow();
    return 0;
}