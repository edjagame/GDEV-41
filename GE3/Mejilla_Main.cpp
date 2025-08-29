#include <raylib.h>
#include <vector>
#include <string>
#include <fstream>

const int WINDOW_WIDTH = 800;
const int WINDOW_HEIGHT = 600;

struct Circle {
    float x;
    float y;
    float radius;
    Color color;
};

void SetKeybinds(int& upKey, int& leftKey, int& downKey, int& rightKey) {
    std::ifstream configFile("config.ini");
    std::string line;

    while (std::getline(configFile, line)) {
        if (line.find("Up=") == 0) {
            upKey = line[3];
        } else if (line.find("Left=") == 0) {
            leftKey = line[5];
        } else if (line.find("Down=") == 0) {
            downKey = line[5];
        } else if (line.find("Right=") == 0) {
            rightKey = line[6];
        }
    }
}

int main() {
    InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Graded Exercise 3");

    int upKey, leftKey, downKey, rightKey;
    SetKeybinds(upKey, leftKey, downKey, rightKey);

    std::vector<Circle> circles = {
        { 80, 90, 60, RED },
        { 700, 100, 60, GREEN },
        { 400, 450, 60, BLUE }
    };

    int currentCircleIndex = 0;
    bool keyboardMode = true;
    float circleSpeed;
    float deltaTime = 0.0f;

    SetTargetFPS(60);

    while (!WindowShouldClose()) {

        deltaTime = GetFrameTime();
        circleSpeed = 200.0f * deltaTime;

        BeginDrawing();
        if (IsKeyPressed(KEY_TAB)) {
            currentCircleIndex = (currentCircleIndex + 1) % circles.size();
        }
        if (IsKeyPressed(KEY_SPACE)) {
            keyboardMode = !keyboardMode;
        }

        for (const Circle& circle : circles) {
            DrawCircle(circle.x, circle.y, circle.radius, circle.color);
        }

        if (keyboardMode) {
            if (IsKeyDown(KEY_UP) || IsKeyDown(upKey)) {
                circles[currentCircleIndex].y -= circleSpeed;
            }
            if (IsKeyDown(KEY_DOWN) || IsKeyDown(downKey)) {
                circles[currentCircleIndex].y += circleSpeed;
            }
            if (IsKeyDown(KEY_LEFT) || IsKeyDown(leftKey)) {
                circles[currentCircleIndex].x -= circleSpeed;
            }
            if (IsKeyDown(KEY_RIGHT) || IsKeyDown(rightKey)) {
                circles[currentCircleIndex].x += circleSpeed;
            }
        } else {
            Vector2 mousePos = GetMousePosition();
            circles[currentCircleIndex].x = mousePos.x;
            circles[currentCircleIndex].y = mousePos.y;
        }

        ClearBackground(BLACK);

        EndDrawing();
    }

    CloseWindow();
    return 0;
}