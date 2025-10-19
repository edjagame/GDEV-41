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
const int QUADTREE_DEPTH = 6;

// Prototypes
struct Ball;
struct QuadTree;
float RandomFloat(float min, float max);
Rectangle GetBallAABB(const Ball &ball);
bool CheckAABBContains(const Rectangle &a, const Rectangle &b);

// Ball struct
struct Ball {
    float radius;
    Vector2 position;

    // Variables for physics
    float mass;
    float inverse_mass;
    Vector2 acceleration;
    Vector2 velocity;

    Color color = WHITE;

    // points to the node in the quadtree this ball is currently in
    QuadTree* currentNode = nullptr;
};

// Quadtree Node struct
struct QuadTree {
    Rectangle boundary;
    QuadTree* children[4] = {nullptr, nullptr, nullptr, nullptr};
    QuadTree* parent = nullptr;
    int depth = 0;
    bool isLeaf = false;
    std::vector<Ball*> balls;

    // constructor (recursively creates children)
    QuadTree(Rectangle boundary, int depth = 0, QuadTree* parent = nullptr)
        : boundary(boundary), parent(parent), depth(depth) {
        if (depth >= QUADTREE_DEPTH) {
            isLeaf = true;
            return;
        }

        float halfWidth = boundary.width / 2.0f;

        children[0] = new QuadTree({boundary.x, boundary.y, halfWidth, halfWidth}, depth + 1, this);
        children[1] = new QuadTree({boundary.x + halfWidth, boundary.y, halfWidth, halfWidth}, depth + 1, this);
        children[2] = new QuadTree({boundary.x, boundary.y + halfWidth, halfWidth, halfWidth}, depth + 1, this);
        children[3] = new QuadTree({boundary.x + halfWidth, boundary.y + halfWidth, halfWidth, halfWidth}, depth + 1, this);
    }

    // destructor 
    ~QuadTree() {
        for (int i = 0; i < 4; ++i) {
            delete children[i];
            children[i] = nullptr;
        }
    }
};

// Positions the root node at the center of the window
Rectangle GetRootNodeBoundary() {
    if (WINDOW_WIDTH >= WINDOW_HEIGHT) {
        return {0, -(WINDOW_WIDTH - WINDOW_HEIGHT) / 2.0f, (float)WINDOW_WIDTH, (float)WINDOW_WIDTH};
    } else {
        return {-(WINDOW_HEIGHT - WINDOW_WIDTH) / 2.0f, 0, (float)WINDOW_HEIGHT, (float)WINDOW_HEIGHT};
    }
}

void InsertBallIntoQuadTree(QuadTree &node, Ball* ball) {
    if (!node.isLeaf) {
        for (int i = 0; i < 4; ++i) {
            if (CheckAABBContains(GetBallAABB(*ball), node.children[i]->boundary)) {
                InsertBallIntoQuadTree(*node.children[i], ball);
                return;
            }
        }
        // if none of the children contain the ball, add it to the current node
        node.balls.push_back(ball);
        ball->currentNode = &node;
    } else {
        //if it reaches leaf node, add ball to this node
        node.balls.push_back(ball);
        ball->currentNode = &node;
    }
}

// Removes a ball from a quadtree node
void RemoveBallFromNode(QuadTree &node, Ball& ball) {
    std::vector<Ball*> &nodeBalls = node.balls;
    nodeBalls.erase(std::remove(nodeBalls.begin(), nodeBalls.end(), &ball), nodeBalls.end());
    ball.currentNode = nullptr;
}

// Clears all balls from the quadtree nodes
void ClearQuadTree(QuadTree &node) {
    node.balls.clear();
    for (int i = 0; i < 4; ++i) {
        if (node.children[i] != nullptr) {
            ClearQuadTree(*node.children[i]);
        }
    }
}

// Function to spawn the 25 small balls
// appends it to the balls vector
void SpawnBalls(std::vector<Ball> &balls) {
    for (int i = 0; i < 25; i++) {
        Ball ball;
        ball.position = {WINDOW_WIDTH / 2.0f, WINDOW_HEIGHT / 2.0f};
        ball.radius = RandomFloat(5.0f, 10.0f);
        ball.velocity = {RandomFloat(-400.0f, 400.0f), RandomFloat(-300.0f, 300.0f)};
        ball.acceleration = {0.0f, 0.0f};
        ball.color = Color {
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

// Function to spawn the big ball
// appends it to the balls vector
void SpawnBigBall(std::vector<Ball> &balls) {
    Ball ball;
    ball.position = {WINDOW_WIDTH / 2.0f, WINDOW_HEIGHT / 2.0f};
    ball.radius = 25.0f;
    ball.velocity = {RandomFloat(-400.0f, 400.0f), RandomFloat(-400.0f, 400.0f)};
    ball.acceleration = {0.0f, 0.0f};
    ball.color = Color {
        static_cast<unsigned char>(RandomFloat(0, 255)),
        static_cast<unsigned char>(RandomFloat(0, 255)),
        static_cast<unsigned char>(RandomFloat(0, 255)),
        255
    };
    ball.mass = 10.0f;
    ball.inverse_mass = 1.0f / ball.mass;
    balls.push_back(ball);
}

// Function to move the ball based on its velocity and acceleration
// no need to update acceleration for now since no external forces are applied
void MoveBall(Ball &ball, float delta_time) {
    // ball.velocity = Vector2Add(ball.velocity, Vector2Scale(ball.acceleration, delta_time));
    ball.velocity = Vector2Subtract(ball.velocity, Vector2Scale(ball.velocity, FRICTION * ball.inverse_mass * delta_time));
    ball.position = Vector2Add(ball.position, Vector2Scale(ball.velocity, delta_time));
}

// gets the AABB of a ball as a rectangle
Rectangle GetBallAABB(const Ball &ball) {
    return {
        ball.position.x - ball.radius,
        ball.position.y - ball.radius,
        ball.radius * 2,
        ball.radius * 2
    };
}

// Helper function to check collision between two circles
bool CheckCircleCollision(const Ball &a, const Ball &b) {
    return Vector2Distance(a.position, b.position) < (a.radius + b.radius);
}


// Helper function to check AABB collision between two rectangles
bool CheckAABBCollision(const Rectangle &a, const Rectangle &b) {
    return (a.x < b.x + b.width &&
            a.x + a.width > b.x &&
            a.y < b.y + b.height &&
            a.y + a.height > b.y);
}

// Helper function to check if AABB a is completely inside AABB b (a is inside b)
bool CheckAABBContains(const Rectangle &a, const Rectangle &b) {
    return (a.x >= b.x &&
            a.x + a.width <= b.x + b.width &&
            a.y >= b.y &&
            a.y + a.height <= b.y + b.height);
}

// Main collision physics function
void Collide(Ball &a, Ball &b, float elasticity) {
    Vector2 normal = Vector2Subtract(b.position, a.position);
    Vector2 relativeVelocity = Vector2Subtract(b.velocity, a.velocity);
    float result = Vector2DotProduct(relativeVelocity, normal);
    if(result < 0) {
        Vector2 n = Vector2Normalize(normal);
        float r = Vector2DotProduct(relativeVelocity, n);

        // impulse is negative now!!!!
        float impulse = - (1 + elasticity) * r / ((Vector2Length(n)) * (a.inverse_mass + b.inverse_mass));
        b.velocity = Vector2Add(b.velocity, Vector2Scale(n, impulse * b.inverse_mass));
        a.velocity = Vector2Subtract(a.velocity, Vector2Scale(n, impulse * a.inverse_mass));
    }
}

void QuadTreeCollision(QuadTree &node, std::vector<Ball*> &parentBalls, float elasticity)  {
    // Get the og size of the stack (only the parent's balls) then insert the nodes balls to the vector
    const size_t base = parentBalls.size();
    parentBalls.insert(parentBalls.end(), node.balls.begin(), node.balls.end());

    // Collide all balls in the vector w each other
    for (int i = base; i < parentBalls.size(); ++i) {
        Ball* a = parentBalls[i];
        for (int j = 0; j < i; ++j) {
            Ball* b = parentBalls[j];
            if (CheckCircleCollision(*a, *b)) {
                Collide(*a, *b, elasticity);
            }
        }
    }

    // Recursify the new vector down to the child nodes
    for (int i = 0; i < 4; ++i) {
        if (node.children[i] != nullptr) {
            QuadTreeCollision(*node.children[i], parentBalls, elasticity);
        }
    }

    // remove the non-parent balls from the vector
    parentBalls.resize(base);

}

void CheckWallCollision(Ball &ball) {

    if(ball.position.x + ball.radius >= WINDOW_WIDTH || ball.position.x - ball.radius <= 0) {
        ball.velocity.x *= -1;
    }
    if(ball.position.y + ball.radius >= WINDOW_HEIGHT || ball.position.y - ball.radius <= 0) {
        ball.velocity.y *= -1;
    }
}

// Helper function that returns a random float value between min and max
float RandomFloat(float min, float max) {
    return min + (max - min) * (static_cast<float>(rand()) / RAND_MAX);
}

void DrawActiveNodes(QuadTree &node) {
    if (!node.balls.empty()) {
        DrawRectangleLinesEx(node.boundary, 2.0f, Color{
            static_cast<unsigned char>(40 + node.depth * 30),
            static_cast<unsigned char>(40 + node.depth * 30),
            static_cast<unsigned char>(40 + node.depth * 30),
            255
        });
    }

    for (int i = 0; i < 4; ++i) {
        if (node.children[i] != nullptr) {
            DrawActiveNodes(*node.children[i]);
        }
    }
}

int main() {
    std::vector<Ball> balls;
    balls.reserve(4096);
    float accumulator = 0;
    float elasticity = 1.0;
    int spacePresses = 0;
    
    QuadTree root = QuadTree(GetRootNodeBoundary());

    InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Quadtrees");
    SetTargetFPS(FPS);

    
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

            // Move all balls
            for(Ball &ball : balls) {
                MoveBall(ball, TIMESTEP);
            }

            // Recreates quadtree every frame (since its easier hehe)
            ClearQuadTree(root);
            for (Ball &ball : balls) {
                ball.currentNode = nullptr;
                InsertBallIntoQuadTree(root, &ball);
            }
            
            // Handle collisions by traversing the quadtree
            std::vector<Ball*> parentBalls;
            parentBalls.reserve(balls.size());
            QuadTreeCollision(root, parentBalls, elasticity);

            // Check wall collisions
            for (Ball &ball : balls) {
                CheckWallCollision(ball);
            }

            accumulator -= TIMESTEP;  
        }
        
        BeginDrawing();
        ClearBackground(BLACK);
        
        DrawActiveNodes(root);

        for(Ball &ball : balls) {
            DrawCircleV(ball.position, ball.radius, ball.color);
        }
        DrawFPS(10, 10);
        DrawText(TextFormat("Balls: %d", balls.size()), 10, 30, 20, WHITE);
        EndDrawing();

    }
    ClearQuadTree(root);
    CloseWindow();
    return 0;
}