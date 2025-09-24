#include <raylib.h>
#include <raymath.h>
#include <vector>

const int WINDOW_WIDTH = 800;
const int WINDOW_HEIGHT = 600;
const float FPS = 60;
const float TIMESTEP = 1 / FPS; 
const float FRICTION = 0.5;

enum class Shape {
    CIRCLE,
    RECTANGLE
};

struct Object {
    Shape shape;
    Vector2 position;
    Color color;
    float mass;
    float inverse_mass;
    Vector2 acceleration;
    Vector2 velocity;
};

struct Ball : Object {
    Shape shape = Shape::CIRCLE;
    float radius;
};

//
struct Box : Object {
    Shape shape = Shape::RECTANGLE;
    float width;
    float height;
};

bool CheckCircleCollision(const Ball &a, const Ball &b) {
    return Vector2Distance(a.position, b.position) < (a.radius + b.radius);
}

void Collide(Object &a, Object &b, Vector2 normal, float elasticity) {
    Vector2 relativeVelocity = Vector2Subtract(b.velocity, a.velocity);
    float result = Vector2DotProduct(relativeVelocity, normal);
    if(result < 0) {
        Vector2 n = Vector2Normalize(normal);
        float r = Vector2DotProduct(relativeVelocity, n);
        float impulse = (1 + elasticity) * r / ((Vector2Length(n)) * (a.inverse_mass + b.inverse_mass));
        a.velocity = Vector2Add(a.velocity, Vector2Scale(n, impulse * a.inverse_mass));
        b.velocity = Vector2Subtract(b.velocity, Vector2Scale(n, impulse * b.inverse_mass));
    }
}

void initPos(std::vector<Ball> &balls) {
    balls[0].position = {200, 300};
    balls[1].position = {518, 300};
    balls[2].position = {622, 300};
    balls[3].position = {570, 270};
    balls[4].position = {570, 330};
}

int main() {
    std::vector<Ball> holes(4);
    std::vector<Box> walls(4);
    std::vector<Ball> balls(5);

    Ball* playerBall = &balls[0];

    for (Ball &hole : holes) {
        hole.radius = 40.0f;
        hole.color = BLACK;
        hole.mass = 0.0f;
        hole.inverse_mass = 0.0f;
        hole.acceleration = Vector2Zero();
        hole.velocity = Vector2Zero();
    }
    holes[0].position = {40, 40};
    holes[1].position = {WINDOW_WIDTH - 40, 40};
    holes[2].position = {40, WINDOW_HEIGHT - 40};
    holes[3].position = {WINDOW_WIDTH - 40, WINDOW_HEIGHT - 40};

    for (Box &wall : walls) {
        wall.color = BROWN;
        wall.mass = 0.0f;
        wall.inverse_mass = 0.0f;
        wall.acceleration = Vector2Zero();
        wall.velocity = Vector2Zero();
    }
    walls[0].width = 40; walls[0].height = WINDOW_HEIGHT-160;
    walls[1].width = 40; walls[1].height = WINDOW_HEIGHT-160;
    walls[2].width = WINDOW_WIDTH-160; walls[2].height = 40; 
    walls[3].width = WINDOW_WIDTH-160; walls[3].height = 40;

    walls[0].position = {0, 80};
    walls[1].position = {WINDOW_WIDTH-40, 80};
    walls[2].position = {80, 0};
    walls[3].position = {80, WINDOW_HEIGHT-40};

    for (Ball &ball : balls) {
        ball.radius = 30.0f;
        ball.color = (&ball == playerBall) ? WHITE : BLUE;
        ball.mass = 0.4f;
        ball.inverse_mass = 1.0f / ball.mass;
        ball.acceleration = Vector2Zero();
        ball.velocity = Vector2Zero();
    }

    initPos(balls);

    InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Physics Demo");

    SetTargetFPS(FPS);

    float accumulator = 0;
    float elasticity = 1.0;

    Vector2 dragOrigin = Vector2Zero();
    Vector2 dragEnd = Vector2Zero();
    bool isDragging = false;
    const float maxDragDistance = 150.0f;
    const float epsilon = 0.5f;
    while (!WindowShouldClose()) {
        float delta_time = GetFrameTime();
        Vector2 forces = Vector2Zero(); 

        bool allStopped = true;
        for (Ball &ball : balls) {
            if (Vector2Length(ball.velocity) > epsilon) {
                allStopped = false;
                break;
            } else {
                ball.velocity = Vector2Zero();
            }
        }
        if (!allStopped) playerBall->color = GRAY ;
        else playerBall->color = WHITE;


        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && allStopped) {
            Vector2 mousePos = GetMousePosition();
            if (Vector2Distance(mousePos, playerBall->position) < playerBall->radius) {
                dragOrigin = playerBall->position;
                isDragging = true;
            }
        }

        if (isDragging && IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
            Vector2 dragDirection = Vector2Normalize(Vector2Subtract(GetMousePosition(), dragOrigin));
            float currentDistance = Vector2Distance(GetMousePosition(), dragOrigin);
            if (currentDistance > maxDragDistance) {
                dragEnd = Vector2Add(dragOrigin, Vector2Scale(dragDirection, maxDragDistance));
            } else {
                dragEnd = GetMousePosition();
            }
        }   

        if (isDragging && IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
            Vector2 dragVector = Vector2Subtract(dragEnd, dragOrigin);
            float dragDistance = Vector2Length(dragVector);
            forces = Vector2Scale(Vector2Normalize(dragVector), - dragDistance * 200);
            isDragging = false;
        }

        if (IsKeyDown(KEY_R)) {
            initPos(balls);
            for (Ball &ball : balls) {
                ball.velocity = Vector2Zero();
            }
        }
    
        playerBall->acceleration = Vector2Scale(forces, playerBall->inverse_mass);
        accumulator += delta_time;

        while(accumulator >= TIMESTEP) {
            for(Ball &ball : balls) {

                ball.velocity = Vector2Add(ball.velocity, Vector2Scale(ball.acceleration, TIMESTEP));
                ball.velocity = Vector2Subtract(ball.velocity, Vector2Scale(ball.velocity, FRICTION * ball.inverse_mass * TIMESTEP));
                ball.position = Vector2Add(ball.position, Vector2Scale(ball.velocity, TIMESTEP));

                for(Ball &otherBall : balls){
                    if (&otherBall == &ball) continue;
                    
                    if (CheckCircleCollision(ball, otherBall)) {
                        Vector2 normal = Vector2Subtract(otherBall.position, ball.position);
                        Collide(ball, otherBall, normal, elasticity);
                    }
                }

                for(Box &wall : walls){
                    if (ball.position.x - ball.radius < wall.position.x + wall.width && 
                        ball.position.x + ball.radius > wall.position.x &&
                        ball.position.y - ball.radius < wall.position.y + wall.height && 
                        ball.position.y + ball.radius > wall.position.y) {
                        Vector2 normal;
                        if (ball.position.x < wall.position.x) normal = {1, 0};
                        if (ball.position.x > wall.position.x + wall.width) normal = {-1, 0};
                        if (ball.position.y < wall.position.y) normal = {0, 1};
                        if (ball.position.y > wall.position.y + wall.height) normal = {0, -1};
                        Collide(ball, wall, normal, elasticity);
                    }
                    
                }
                for(Ball &hole : holes){
                    if (CheckCircleCollision(ball, hole)) {
                        if (&ball == playerBall) {
                            playerBall->position = {200, 300};
                            playerBall->velocity = Vector2Zero();
                        } else {
                            ball.position = {-100, -100};
                            ball.velocity = Vector2Zero();
                        }
                    }
                }
                
                // Negates the velocity at x and y if the object hits a wall. (Basic Collision Detection)
                if(ball.position.x + ball.radius >= WINDOW_WIDTH || ball.position.x - ball.radius <= 0) {
                    ball.velocity.x *= -1;
                }
                if(ball.position.y + ball.radius >= WINDOW_HEIGHT || ball.position.y - ball.radius <= 0) {
                    ball.velocity.y *= -1;
                }
            }
            accumulator -= TIMESTEP;  
        }
        
        
        ClearBackground(GREEN);
        BeginDrawing();
        
        for(Ball &ball : balls) {
            DrawCircleV(ball.position, ball.radius, ball.color);
        }
        for(Box &wall : walls) {
            DrawRectangle(wall.position.x, wall.position.y, wall.width, wall.height, wall.color);
        }
        for(Ball &hole : holes) {
            DrawCircleV(hole.position, hole.radius, hole.color);
        }
        if (isDragging && IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
            float dragDistance = Vector2Distance(dragOrigin, dragEnd);
            float thickness = Lerp(10.0f, 2.0f, dragDistance / maxDragDistance);
            DrawLineEx(dragOrigin, dragEnd, thickness, GRAY);
        }
        EndDrawing();
    }
    CloseWindow();
    return 0;
}