#include <raylib.h>
#include <vector>

const int WINDOW_WIDTH = 800;
const int WINDOW_HEIGHT = 600;
    
int main() {
    InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Graded Exercise 2");

    float rectOriginX = 10;
    float rectOriginY = 20;
    float rectWidth = 150;
    float rectHeight = 100;
    float rectSpeedX = 0.5f;
    float rectSpeedY = 0.5f;

    std::vector<Color> rectColors = { RED, GREEN, BLUE, YELLOW, PURPLE };
    int currentColorIndex = 0;

    while (!WindowShouldClose()) {
        BeginDrawing();

        ClearBackground(BLACK);
        DrawRectangle(rectOriginX, rectOriginY, rectWidth, rectHeight, rectColors[currentColorIndex]);

        rectOriginX += rectSpeedX;
        rectOriginY += rectSpeedY;

        if (rectOriginX + rectWidth > WINDOW_WIDTH || rectOriginX < 0) {
            rectSpeedX = -rectSpeedX;
            currentColorIndex = (currentColorIndex + 1) % rectColors.size();
        }
        
        if (rectOriginY + rectHeight > WINDOW_HEIGHT || rectOriginY < 0) {
            rectSpeedY = -rectSpeedY;
            currentColorIndex = (currentColorIndex + 1) % rectColors.size();
        }

        EndDrawing();
    }

    CloseWindow();
    return 0;
}