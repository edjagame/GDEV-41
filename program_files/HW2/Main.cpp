#include <raylib.h>
#include <raymath.h>
#include <random>
#include <fstream>
#include <string>

const int WINDOW_WIDTH = 800;
const int WINDOW_HEIGHT = 600;
const int MAX_PARTICLES = 1000;

struct Particle {
    bool isActive = false;
    Vector2 position;
    Vector2 direction;
    float speed;
    float lifeTime;
    float currentLifeTime;
    Color color;
};

float RandomFloat(float min, float max) {
    return min + (max - min) * (static_cast<float>(rand()) / RAND_MAX);
}

void EmitParticle(Particle &particle, bool keyboardMode) {
    particle.isActive = true;
    if (keyboardMode) {
        particle.position = { WINDOW_WIDTH / 2.0f, WINDOW_HEIGHT };
        particle.direction = {RandomFloat(-1.0f, 1.0f), -1.0f};
        particle.lifeTime = RandomFloat(2.0f, 5.0f);
    } else {
        particle.position = GetMousePosition();
        particle.direction = {RandomFloat(-1.0f, 1.0f), RandomFloat(-1.0f, 1.0f)};
        particle.lifeTime = RandomFloat(0.5f, 2.0f);
    }
    particle.currentLifeTime = particle.lifeTime;
    particle.speed = RandomFloat(50.0f, 100.0f);
    particle.color = { 
        static_cast<unsigned char>(rand() % 256), 
        static_cast<unsigned char>(rand() % 256), 
        static_cast<unsigned char>(rand() % 256), 
        static_cast<unsigned char>(255) 
    };
}

Particle& FindNextParticle(Particle* particles) {
    for (int i = 0; i < MAX_PARTICLES; i++) {
        if (!particles[i].isActive) {
            return particles[i];
        }
    }
    return particles[0]; // if max particles fsr
}

struct Keybind {
    std::string name;
    bool isKeyboard;
    int key;
};

void SetKeybinds(Keybind& keybind){
    std::ifstream config("config.ini");
    std::string line;
    if (config.is_open()) {
        while (std::getline(config, line)) {
            if (line.find(keybind.name) == 0) {
                if(line.substr(line.find('=') + 1, 1) == "K") {
                    keybind.isKeyboard = true;
                } else {
                    keybind.isKeyboard = false;
                }
                keybind.key = std::stoi(line.substr(line.find('|') + 1));
            }
        }
        config.close();
    }
}

bool IsButtonDown(const Keybind& keybind) {
    if (keybind.isKeyboard) {
        return IsKeyDown(keybind.key);
    } else {
        return IsMouseButtonDown(keybind.key);
    }
}

int main() {
    InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Homework 2");

    std::vector<Keybind> keybinds = {
        {"KeyboardRateUp", true, KEY_RIGHT},
        {"KeyboardRateDown", true, KEY_LEFT},
        {"MouseRateUp", true, KEY_UP},
        {"MouseRateDown", true, KEY_DOWN},
        {"KeyboardButton", true, KEY_SPACE},
        {"MouseButton", false, MOUSE_BUTTON_LEFT}
    };
    for (Keybind& kb : keybinds) {
        SetKeybinds(kb);
    }

    Particle* particles = new Particle[MAX_PARTICLES];
    float keyboardParticleRate = 25; // particles per second
    float mouseParticleRate = 25; // particles per second
    float rateChangeSpeed = 10.0f; 
    float keyboardAccumulator = 0.0f;
    float mouseAccumulator = 0.0f;

    SetTargetFPS(60);

    while (!WindowShouldClose()) {

        float deltaTime = GetFrameTime();

        if (IsButtonDown(keybinds[0])) keyboardParticleRate += rateChangeSpeed * deltaTime;
        if (IsButtonDown(keybinds[1])) keyboardParticleRate -= rateChangeSpeed * deltaTime;
        if (IsButtonDown(keybinds[2])) mouseParticleRate += rateChangeSpeed * deltaTime;
        if (IsButtonDown(keybinds[3])) mouseParticleRate -= rateChangeSpeed * deltaTime;

        keyboardParticleRate = Clamp(keyboardParticleRate, 1, 50);
        mouseParticleRate = Clamp(mouseParticleRate, 1, 50);

        if (IsButtonDown(keybinds[4])) {
            keyboardAccumulator += deltaTime;
            if (keyboardAccumulator >= 1.0f / keyboardParticleRate) {
                EmitParticle(FindNextParticle(particles), true);
                keyboardAccumulator -= 1.0f / keyboardParticleRate;
            }
        }
        if (IsButtonDown(keybinds[5])) {
            mouseAccumulator += deltaTime;
            if (mouseAccumulator >= 1.0f / mouseParticleRate) {
                EmitParticle(FindNextParticle(particles), false);
                mouseAccumulator -= 1.0f / mouseParticleRate;
            }
        }

        for (int i = 0; i < MAX_PARTICLES; i++) {
            if (particles[i].isActive) {
                particles[i].currentLifeTime -= deltaTime;
                if (particles[i].currentLifeTime <= 0) {
                    particles[i].isActive = false;
                } else {
                    particles[i].position.x += particles[i].direction.x * particles[i].speed * deltaTime;
                    particles[i].position.y += particles[i].direction.y * particles[i].speed * deltaTime;
                }
                particles[i].color.a = static_cast<unsigned char>((particles[i].currentLifeTime / particles[i].lifeTime) * 255);
            }
        }
        BeginDrawing();
        ClearBackground(BLACK);

        for (int i = 0; i < MAX_PARTICLES; i++) {
            if (particles[i].isActive) {
                DrawCircleV(particles[i].position, 5.0f, particles[i].color);
            }
        }
        EndDrawing();
    }

    delete[] particles;
    CloseWindow();
    return 0;
}