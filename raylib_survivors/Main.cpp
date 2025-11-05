#include <vector>
#include <raylib.h>
#include <raymath.h>
#include "entt.hpp"

const int WINDOW_WIDTH = 1280;
const int WINDOW_HEIGHT = 720;
const float FPS = 60.0f;
const float TIMESTEP = 1 / FPS; 

struct PlayerTag {/* If an entity has this component, it is a player */};
struct EnemyTag {/* If an entity has this component, it is an enemy */};
struct WeaponTag {/* If an entity has this component, it is a weapon */};
struct ProjectileTag {/* If an entity has this component, it is a projectile */};

struct RectangleComponent { Rectangle rectangle; };
struct CircleComponent { Vector2 center; float radius; };
struct PositionComponent { Vector2 position; };
struct VelocityComponent { Vector2 velocity; };
struct ColorComponent { Color color; };
struct MoveSpeedComponent { float speed; };
struct SpriteComponent { Texture2D sprite; Vector2 spriteSize; };

struct AimDirectionComponent { Vector2 direction; };
struct FireRateComponent { float fireRate; float fireRateAccumulator; };
struct LifetimeComponent { float remaining; };
struct ProjectileComponent { CircleComponent hitbox; MoveSpeedComponent speed; float lifetime;  };

void InitPlayer(entt::registry& registry, entt::entity e) {
    Vector2 center = {WINDOW_WIDTH / 2.0f, WINDOW_HEIGHT / 2.0f};
    float moveSpeed = 200.0f;
    Texture2D sprite = LoadTexture("assets/player.png");
    Vector2 spriteSize = {50, 50};

    registry.emplace<PlayerTag>(e);
    registry.emplace<PositionComponent>(e, center);
    registry.emplace<VelocityComponent>(e, Vector2Zero());
    registry.emplace<ColorComponent>(e, WHITE);
    registry.emplace<MoveSpeedComponent>(e, moveSpeed);
    registry.emplace<SpriteComponent>(e, sprite, spriteSize);
}

void InitGun (entt::registry& registry, entt::entity e) {
    registry.emplace<WeaponTag>(e);
    registry.emplace<PositionComponent>(e, Vector2Zero());
    registry.emplace<AimDirectionComponent>(e, Vector2Zero());
    registry.emplace<FireRateComponent>(e, 0.5f, 0.0f); // fire rate of 0.5 seconds
    registry.emplace<ProjectileComponent>(e, CircleComponent{Vector2Zero(), 5.0f}, MoveSpeedComponent{400.0f}, 2.0f);
}

// Handles player input and updates velocity component
void PlayerInputSystem(entt::registry& registry) {
    auto viewPlayer = registry.view<PlayerTag, VelocityComponent, MoveSpeedComponent>();
    for (auto e : viewPlayer) {
        Vector2& velocity = viewPlayer.get<VelocityComponent>(e).velocity;
        float& speed = viewPlayer.get<MoveSpeedComponent>(e).speed;

        velocity = Vector2{0, 0};
        if (IsKeyDown(KEY_W)) velocity.y -= speed;
        if (IsKeyDown(KEY_S)) velocity.y += speed;
        if (IsKeyDown(KEY_A)) velocity.x -= speed;
        if (IsKeyDown(KEY_D)) velocity.x += speed;
    }
}

// Updates position based on velocity and delta time
void MovementSystem(entt::registry& registry, float delta_time) {
    auto viewMovement = registry.view<PositionComponent, VelocityComponent>();
    for (auto e : viewMovement) {
        Vector2& position = viewMovement.get<PositionComponent>(e).position;
        Vector2& velocity = viewMovement.get<VelocityComponent>(e).velocity;

        position.x += velocity.x * delta_time;
        position.y += velocity.y * delta_time;
    }

    auto spriteView = registry.view<PositionComponent, SpriteComponent>();
    for (auto e : spriteView) {
        Vector2& position = spriteView.get<PositionComponent>(e).position;
        Vector2& spriteSize = spriteView.get<SpriteComponent>(e).spriteSize;
        float halfW = spriteSize.x * 0.5f;
        float halfH = spriteSize.y * 0.5f;
        position.x = Clamp(position.x, halfW, WINDOW_WIDTH - halfW);
        position.y = Clamp(position.y, halfH, WINDOW_HEIGHT - halfH);
    }
}

void DrawSystem(entt::registry& registry) {
    auto view = registry.view<PositionComponent, SpriteComponent, ColorComponent>();
    for (auto e : view) {
        Vector2& pos = view.get<PositionComponent>(e).position;
        Texture2D& sprite = view.get<SpriteComponent>(e).sprite;
        Vector2& spriteSize = view.get<SpriteComponent>(e).spriteSize;
        Color& color = view.get<ColorComponent>(e).color;

        Rectangle source = { 0.0f, 0.0f, (float)sprite.width, (float)sprite.height };
        Rectangle dest = { pos.x, pos.y, spriteSize.x, spriteSize.y };
        Vector2 origin = { spriteSize.x * 0.5f, spriteSize.y * 0.5f };
        DrawTexturePro(sprite, source, dest, origin, 0.0f, color);
    }

    // Draw projectiles as circles 
    auto projectileView = registry.view<PositionComponent, CircleComponent, ColorComponent>(entt::exclude<PlayerTag>);
    for (auto e : projectileView) {
        Vector2& pos = projectileView.get<PositionComponent>(e).position;
        CircleComponent& circle = projectileView.get<CircleComponent>(e);
        Color& color = projectileView.get<ColorComponent>(e).color;

        DrawCircleV(pos, circle.radius, color);
    }
}


// system that aims from player to mouse position
// note: needs to be called after InitPlayer
void AimSystem(entt::registry& registry) {
    
    // Get player position
    Vector2 playerPos = Vector2Zero();
    auto playerView = registry.view<PlayerTag, PositionComponent>();
    // should only be one player 
    for (auto e : playerView) {
        playerPos = playerView.get<PositionComponent>(e).position;
    }

    auto view = registry.view<AimDirectionComponent, PositionComponent>();
    for (auto e : view) {
        Vector2& aimDir = view.get<AimDirectionComponent>(e).direction;
        Vector2& position = view.get<PositionComponent>(e).position;

        position = playerPos;
        Vector2 mousePos = GetMousePosition();
        aimDir = Vector2Normalize(Vector2Subtract(mousePos, position));
    }
}

void FireSystem(entt::registry& registry, float delta_time) {
    auto view = registry.view<FireRateComponent, AimDirectionComponent, PositionComponent, ProjectileComponent>();
    for (auto e : view) {
        float& fireRate = view.get<FireRateComponent>(e).fireRate;
        float& fireRateAccumulator = view.get<FireRateComponent>(e).fireRateAccumulator;
        Vector2& aimDir = view.get<AimDirectionComponent>(e).direction;
        Vector2& position = view.get<PositionComponent>(e).position;
        ProjectileComponent& projectileData = view.get<ProjectileComponent>(e);

        fireRateAccumulator += delta_time;

        if (fireRateAccumulator >= fireRate) {
            fireRateAccumulator = 0.0f;

            entt::entity projectile = registry.create();
            
            float projectileSpeed = projectileData.speed.speed;
            float lifetime = projectileData.lifetime;
            Vector2 projectileVelocity = Vector2Scale(aimDir, projectileSpeed);
            Vector2 projectilePosition = position;

            registry.emplace<ProjectileTag>(projectile);
            registry.emplace<PositionComponent>(projectile, projectilePosition);
            registry.emplace<VelocityComponent>(projectile, projectileVelocity);
            registry.emplace<CircleComponent>(projectile, CircleComponent{projectileData.hitbox});
            registry.emplace<MoveSpeedComponent>(projectile, MoveSpeedComponent{projectileSpeed});
            registry.emplace<ColorComponent>(projectile, ColorComponent{RED});
            registry.emplace<LifetimeComponent>(projectile, LifetimeComponent{ lifetime });
        }
    }
}

void DespawnProjectilesSystem (entt::registry& registry, float delta_time) {
    auto view = registry.view<ProjectileTag, LifetimeComponent>();
    for (auto e : view) {
        float& remaining = view.get<LifetimeComponent>(e).remaining;
        remaining -= delta_time;
        if (remaining <= 0.0f) {
            registry.destroy(e);
        }
    }
}

void UnloadResources(entt::registry& registry) {
    auto view = registry.view<SpriteComponent>();
    for (auto e : view) {
        Texture2D& sprite = view.get<SpriteComponent>(e).sprite;
        UnloadTexture(sprite);
    }
}

entt::entity FindPlayerEntity(entt::registry& registry) {
    auto view = registry.view<PlayerTag>();
    for (auto e : view) {
        return e; 
    }
    return entt::null;
}

int main() {
    InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Raylib Survivors");
    SetTargetFPS(FPS);

    entt::registry registry;
    entt::entity player_entity = registry.create();
    InitPlayer(registry, player_entity);
    entt::entity gun_entity = registry.create();
    InitGun(registry, gun_entity);

    while (!WindowShouldClose()) {
        float delta_time = GetFrameTime();


        PlayerInputSystem(registry);

        MovementSystem(registry, delta_time);
        AimSystem(registry);
        FireSystem(registry, delta_time);
        DespawnProjectilesSystem(registry, delta_time);

        BeginDrawing();
        ClearBackground(BLACK);
        DrawSystem(registry);
        EndDrawing();
    }

    UnloadResources(registry);
    CloseWindow();
    return 0;
}