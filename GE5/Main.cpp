#include <raylib.h>
#include <raymath.h>
#include <string>
#include <vector>
#include <iostream>
#include <algorithm>

const int WINDOW_WIDTH = 1280;
const int WINDOW_HEIGHT = 720;
const float FPS = 60;
const float TIMESTEP = 1 / FPS; 
const float FRICTION = 0.0;

struct Ball {
    float radius;
    Vector2 position;
    Color color;
    float mass;
    float inverse_mass;
    Vector2 acceleration;
    Vector2 velocity;

    std::vector<Ball*> collisions;
};

struct GridCell {
    std::vector<Ball*> balls;
    int x, y;
    int numBalls;
    int width;
};

void InitGrid(std::vector<GridCell> &grid, int cellSize) {
    for(int y = 0; y < WINDOW_HEIGHT; y += cellSize) {
        for(int x = 0; x < WINDOW_WIDTH; x += cellSize) {
            GridCell cell;
            cell.x = x / cellSize;
            cell.y = y / cellSize;
            cell.width = cellSize;
            grid.push_back(cell);
        }
    }
}

void AssignBallToGrid(std::vector<GridCell> &grid, Ball &ball, int cellSize) {
    int index1 = (int)(ball.position.y + ball.radius) / cellSize * (WINDOW_WIDTH / cellSize) + (int)(ball.position.x) / cellSize;
    int index2 = (int)(ball.position.y - ball.radius) / cellSize * (WINDOW_WIDTH / cellSize) + (int)(ball.position.x) / cellSize;
    int index3 = (int)(ball.position.y) / cellSize * (WINDOW_WIDTH / cellSize) + ((int)ball.position.x + ball.radius) / cellSize;
    int index4 = (int)(ball.position.y) / cellSize * (WINDOW_WIDTH / cellSize) + ((int)ball.position.x - ball.radius) / cellSize;
    
    std::vector<GridCell*> assignedCells;
    if(index1 >= 0 && index1 < grid.size()) assignedCells.push_back(&grid[index1]);
    if(index2 >= 0 && index2 < grid.size()) assignedCells.push_back(&grid[index2]);
    if(index3 >= 0 && index3 < grid.size()) assignedCells.push_back(&grid[index3]);
    if(index4 >= 0 && index4 < grid.size()) assignedCells.push_back(&grid[index4]);

    for(GridCell* cell : assignedCells) {
        if(std::find(cell->balls.begin(), cell->balls.end(), &ball) == cell->balls.end()) {
            cell->balls.push_back(&ball);
        }
    }
}

float RandomFloat(float min, float max) {
    return min + (max - min) * (static_cast<float>(rand()) / RAND_MAX);
}

void SpawnBalls(std::vector<Ball> &balls) {
    for (int i = 0; i < 1; i++) {
        Ball ball;
        ball.position = {WINDOW_WIDTH / 2.0f, WINDOW_HEIGHT / 2.0f};
        ball.radius = RandomFloat(5.0f, 10.0f);
        ball.velocity = {RandomFloat(-400.0f, 400.0f), RandomFloat(-300.0f, 300.0f)};
        ball.acceleration = {0.0f, 0.0f};
        ball.color = {
            static_cast<unsigned char>(RandomFloat(0, 255)),
            static_cast<unsigned char>(RandomFloat(0, 255)),
            static_cast<unsigned char>(RandomFloat(0, 255)),
            255
        };
        ball.mass = 1.0f;
        ball.inverse_mass = 1.0f / ball.mass;

        balls.push_back(ball);
    }
}

void SpawnBigBall(std::vector<Ball> &balls) {
    Ball ball;
    ball.position = {WINDOW_WIDTH / 2.0f, WINDOW_HEIGHT / 2.0f};
    ball.radius = 25.0f;
    ball.velocity = {RandomFloat(-400.0f, 400.0f), RandomFloat(-400.0f, 400.0f)};
    ball.acceleration = {0.0f, 0.0f};
    ball.color = {
        static_cast<unsigned char>(RandomFloat(0, 255)),
        static_cast<unsigned char>(RandomFloat(0, 255)),
        static_cast<unsigned char>(RandomFloat(0, 255)),
        255
    };
    ball.mass = 10.0f;
    ball.inverse_mass = 1.0f / ball.mass;
    balls.push_back(ball);
}

bool CheckCircleCollision(const Ball &a, const Ball &b) {
    return Vector2Distance(a.position, b.position) < (a.radius + b.radius);
}

void Collide(Ball &a, Ball &b, Vector2 normal, float elasticity) {
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

void MoveBall(Ball &ball, float delta_time) {
    ball.velocity = Vector2Add(ball.velocity, Vector2Scale(ball.acceleration, delta_time));
    ball.velocity = Vector2Subtract(ball.velocity, Vector2Scale(ball.velocity, FRICTION * ball.inverse_mass * delta_time));
    ball.position = Vector2Add(ball.position, Vector2Scale(ball.velocity, delta_time));
}

void CheckWallCollision(Ball &ball) {
    if(ball.position.x + ball.radius >= WINDOW_WIDTH || ball.position.x - ball.radius <= 0) {
        ball.velocity.x *= -1;
    }
    if(ball.position.y + ball.radius >= WINDOW_HEIGHT || ball.position.y - ball.radius <= 0) {
        ball.velocity.y *= -1;
    }
}

int main() {
    std::vector<Ball> balls;

    InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Physics Demo");

    SetTargetFPS(FPS);

    float accumulator = 0;
    float elasticity = 1.0;

    std::vector<GridCell> grid;

    const int cellSize = 110;
    InitGrid(grid, cellSize);

    int spacePresses = 0;

    while (!WindowShouldClose()) {
        float delta_time = GetFrameTime();
        Vector2 forces = Vector2Zero(); 
        
        
        if(IsKeyPressed(KEY_SPACE)) {
            if (spacePresses < 10) {
                SpawnBalls(balls);
                spacePresses++;
            } else {
                SpawnBigBall(balls);
                spacePresses = 0;
            }
        }

        accumulator += delta_time;
        
        while(accumulator >= TIMESTEP) {
            for(Ball &ball : balls) {
                MoveBall(ball, TIMESTEP);
                AssignBallToGrid(grid, ball, cellSize);
                ball.collisions.clear();
            }
            
            for(GridCell &cell : grid) {

                int n = cell.balls.size();
                if (n <= 1) {
                    cell.numBalls = n;
                    cell.balls.clear();
                    continue;
                }

                for (Ball* a : cell.balls) {
                    for (Ball* b : cell.balls) {
                        if (std::find(a->collisions.begin(), a->collisions.end(), b) != a->collisions.end()) continue;
                        if (a != b && CheckCircleCollision(*a, *b)) {
                            Vector2 normal = Vector2Subtract(b->position, a->position);
                            Collide(*a, *b, normal, elasticity);
                            a->collisions.push_back(b);
                            b->collisions.push_back(a);
                        }
                    }
                }

                cell.numBalls = n;
                cell.balls.clear();
            }

            for (Ball &ball : balls) {
                CheckWallCollision(ball);
            }
            accumulator -= TIMESTEP;  
        }
        
        BeginDrawing();
        ClearBackground(BLACK);

        for(GridCell &cell : grid) {
            DrawRectangleLines(cell.x * cellSize, cell.y * cellSize, cell.width * cellSize, cell.width * cellSize, GRAY);
            DrawText(TextFormat("(%d, %d)", cell.x, cell.y), cell.x * cellSize + 5, cell.y * cellSize + 5, 10, GRAY);
            DrawText(TextFormat("%d", (int)cell.numBalls), cell.x * cellSize + 5, cell.y * cellSize + 20, 10, GRAY);
        }

        for(Ball &ball : balls) {
            DrawCircleV(ball.position, ball.radius, ball.color);
        }

        DrawFPS(10, 10);
        DrawText(TextFormat("Balls: %d", balls.size()), 10, 30, 20, WHITE);
        EndDrawing();

    }
    CloseWindow();
    return 0;
}