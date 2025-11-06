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
struct ProjectileTag { bool isPlayerProjectile;/* If an entity has this component, it is a projectile */};

struct HealthComponent { float health; };
struct IFramesComponent { bool isInvuln; float invulnTime; float invulnAccumulator; };

// Basic components for position, velocity, and movement
struct PositionComponent { Vector2 position; };
struct VelocityComponent { Vector2 velocity; };
struct MoveSpeedComponent { float speed; };

// Shape components for drawing
struct RectangleHitboxComponent { Vector2 size; };
struct CircleHitboxComponent { float radius; };
struct ContactDamageComponent { float damage; };

// Drawing related components
struct SpriteComponent { Texture2D sprite; Vector2 spriteSize; };
struct ColorComponent { Color originalColor; Color currentColor = originalColor; };
struct HitFlashComponent { bool isHit; float flashDuration; float flashAccumulator; Color flashColor; };

// Weapon data components
struct AimDirectionComponent { Vector2 direction; };
struct FireRateComponent { float fireRate; float fireRateAccumulator; };
struct LifetimeComponent { float remaining; };

// Projectile data for spawning projectiles (used by weapons)
struct ProjectileComponent {    CircleHitboxComponent projectileHitbox; 
                                ContactDamageComponent projectileDamage;
                                MoveSpeedComponent projectileSpeed; 
                                LifetimeComponent projectileLifetime;
                            };


void InitPlayer(entt::registry& registry, entt::entity e) {
    Vector2 center = {WINDOW_WIDTH / 2.0f, WINDOW_HEIGHT / 2.0f};
    float health = 100.0f;
    float moveSpeed = 200.0f;
    Texture2D sprite = LoadTexture("assets/player.png");
    Vector2 spriteSize = {50, 50};
    
    registry.emplace<PlayerTag>(e);
    registry.emplace<IFramesComponent>(e, IFramesComponent{false, 0.5f, 0.0f});
    registry.emplace<CircleHitboxComponent>(e, CircleHitboxComponent{20.0f});
    registry.emplace<HealthComponent>(e, health);
    registry.emplace<PositionComponent>(e, center);
    registry.emplace<VelocityComponent>(e, Vector2Zero());
    registry.emplace<ColorComponent>(e, WHITE);
    registry.emplace<MoveSpeedComponent>(e, moveSpeed);
    registry.emplace<SpriteComponent>(e, sprite, spriteSize);
    registry.emplace<HitFlashComponent>(e, false, 0.1f, 0.0f, RED);
}

void InitEnemy(entt::registry& registry, entt::entity e, 
                Vector2 position = {100.0f, 100.0f}, 
                float health = 50, 
                float moveSpeed = 100.0f) {

    registry.emplace<EnemyTag>(e);
    registry.emplace<HealthComponent>(e, health);
    registry.emplace<ContactDamageComponent>(e, 10.0f);
    registry.emplace<PositionComponent>(e, position);
    registry.emplace<VelocityComponent>(e, Vector2Zero());
    registry.emplace<AimDirectionComponent>(e, Vector2Zero());
    registry.emplace<ColorComponent>(e, YELLOW);
    registry.emplace<MoveSpeedComponent>(e, moveSpeed);
    registry.emplace<CircleHitboxComponent>(e, CircleHitboxComponent{20.0f});
    registry.emplace<HitFlashComponent>(e, false, 0.1f, 0.0f, RED);
}

void InitGun (entt::registry& registry, entt::entity e) {
    registry.emplace<WeaponTag>(e);
    registry.emplace<PositionComponent>(e, Vector2Zero());
    registry.emplace<AimDirectionComponent>(e, Vector2Zero());
    registry.emplace<FireRateComponent>(e, 0.5f, 0.0f); // fire rate of 0.5 seconds
    registry.emplace<ProjectileComponent>(  e, 
                                            CircleHitboxComponent{5.0f}, 
                                            10.0f,
                                            MoveSpeedComponent{400.0f}, 
                                            2.0f);
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

void PlayerMovementSystem(entt::registry& registry, float delta_time) {
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

void EnemyMovementSystem(entt::registry& registry, float delta_time) {
    auto enemyView = registry.view<EnemyTag, PositionComponent, MoveSpeedComponent, AimDirectionComponent>();
    auto playerView = registry.view<PlayerTag, PositionComponent>();

    // Get player position
    Vector2 playerPos = Vector2Zero();
    for (auto p : playerView) {
        playerPos = playerView.get<PositionComponent>(p).position;
    }

    for (auto e : enemyView) {
        Vector2& enemyPos = enemyView.get<PositionComponent>(e).position;
        float& speed = enemyView.get<MoveSpeedComponent>(e).speed;
        Vector2& aimDir = enemyView.get<AimDirectionComponent>(e).direction;

        aimDir = Vector2Normalize(Vector2Subtract(playerPos, enemyPos));
        Vector2 velocity = Vector2Scale(aimDir, speed);

        enemyPos.x += velocity.x * delta_time;
        enemyPos.y += velocity.y * delta_time;

    }
}

void DrawSystem(entt::registry& registry) {
    //Draw Sprites
    auto view = registry.view<PositionComponent, SpriteComponent, ColorComponent>();
    for (auto e : view) {
        Vector2& pos = view.get<PositionComponent>(e).position;
        Texture2D& sprite = view.get<SpriteComponent>(e).sprite;
        Vector2& spriteSize = view.get<SpriteComponent>(e).spriteSize;
        Color& color = view.get<ColorComponent>(e).currentColor;

        Rectangle source = { 0.0f, 0.0f, (float)sprite.width, (float)sprite.height };
        Rectangle dest = { pos.x, pos.y, spriteSize.x, spriteSize.y };
        Vector2 origin = { spriteSize.x * 0.5f, spriteSize.y * 0.5f };
        DrawTexturePro(sprite, source, dest, origin, 0.0f, color);
    }

    // Draw Hitboxes
    auto shapeView = registry.view<PositionComponent, CircleHitboxComponent, ColorComponent>();
    for (auto e : shapeView) {
        Vector2& pos = shapeView.get<PositionComponent>(e).position;
        CircleHitboxComponent& circle = shapeView.get<CircleHitboxComponent>(e);
        Color& color = shapeView.get<ColorComponent>(e).currentColor;

        DrawCircleV(pos, circle.radius, color);
    }

    // Draw projectiles
    auto projectileView = registry.view<ProjectileTag, PositionComponent, CircleHitboxComponent, ColorComponent>();
    for (auto e : projectileView) {
        Vector2& pos = projectileView.get<PositionComponent>(e).position;
        CircleHitboxComponent& circle = projectileView.get<CircleHitboxComponent>(e);
        Color& color = projectileView.get<ColorComponent>(e).currentColor;
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

    auto weaponView = registry.view<WeaponTag, AimDirectionComponent, PositionComponent>();
    for (auto e : weaponView) {
        Vector2& aimDir = weaponView.get<AimDirectionComponent>(e).direction;
        Vector2& position = weaponView.get<PositionComponent>(e).position;

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

            float projectileSpeed = projectileData.projectileSpeed.speed;
            float lifetime = projectileData.projectileLifetime.remaining;
            float damage = projectileData.projectileDamage.damage;

            Vector2 projectileVelocity = Vector2Scale(aimDir, projectileSpeed);
            Vector2 projectilePosition = position;

            registry.emplace<ProjectileTag>(projectile, true);
            registry.emplace<PositionComponent>(projectile, projectilePosition);
            registry.emplace<VelocityComponent>(projectile, projectileVelocity);
            registry.emplace<CircleHitboxComponent>(projectile, CircleHitboxComponent{projectileData.projectileHitbox});
            registry.emplace<MoveSpeedComponent>(projectile, MoveSpeedComponent{projectileSpeed});
            registry.emplace<ColorComponent>(projectile, ColorComponent{RED});
            registry.emplace<LifetimeComponent>(projectile, LifetimeComponent{ lifetime });
            registry.emplace<ContactDamageComponent>(projectile, ContactDamageComponent{ damage });
        }
    }
}

void HitSystem(entt::registry& registry) {
    auto projectileView = registry.view<ProjectileTag, PositionComponent, CircleHitboxComponent, ContactDamageComponent>();
    auto enemyView = registry.view<EnemyTag, PositionComponent, CircleHitboxComponent, HealthComponent, ContactDamageComponent, HitFlashComponent>();
    auto playerView = registry.view<PlayerTag, PositionComponent, CircleHitboxComponent, HealthComponent, IFramesComponent, HitFlashComponent>();
    // naive implementation, TODO: switch to uniform grid
    for (auto proj : projectileView) {
        Vector2& projPos = projectileView.get<PositionComponent>(proj).position;
        CircleHitboxComponent& projHitbox = projectileView.get<CircleHitboxComponent>(proj);
        float projDamage = projectileView.get<ContactDamageComponent>(proj).damage;

        for (auto enemy : enemyView) {
            Vector2& enemyPos = enemyView.get<PositionComponent>(enemy).position;
            CircleHitboxComponent& enemyHitbox = enemyView.get<CircleHitboxComponent>(enemy);
            HealthComponent& enemyHealth = enemyView.get<HealthComponent>(enemy);
            HitFlashComponent& enemyHitFlash = enemyView.get<HitFlashComponent>(enemy);
            
            float dist = Vector2Distance(projPos, enemyPos);
            if (dist <= (projHitbox.radius + enemyHitbox.radius)) {
                enemyHealth.health -= projDamage;
                enemyHitFlash.isHit = true;
                registry.destroy(proj);
                break;
            }
        }
    }

    for (auto player : playerView) {
        Vector2& playerPos = playerView.get<PositionComponent>(player).position;
        CircleHitboxComponent& playerHitbox = playerView.get<CircleHitboxComponent>(player);
        HealthComponent& playerHealth = playerView.get<HealthComponent>(player);
        IFramesComponent& playerIFrames = playerView.get<IFramesComponent>(player);
        HitFlashComponent& playerHitFlash = playerView.get<HitFlashComponent>(player);

        for (auto enemy : enemyView) {
            Vector2& enemyPos = enemyView.get<PositionComponent>(enemy).position;
            CircleHitboxComponent& enemyHitbox = enemyView.get<CircleHitboxComponent>(enemy);
            ContactDamageComponent& enemyDamage = enemyView.get<ContactDamageComponent>(enemy);

            float dist = Vector2Distance(playerPos, enemyPos);
            if (dist <= (playerHitbox.radius + enemyHitbox.radius) && !playerIFrames.isInvuln) {
                playerHealth.health -= enemyDamage.damage;
                playerHitFlash.isHit = true;
                playerIFrames.isInvuln = true;
                playerIFrames.invulnAccumulator = 0.0f;
                break;
            }
        }
    }
}

void DefeatedEnemiesSystem(entt::registry& registry) {
    auto view = registry.view<EnemyTag, HealthComponent>();
    for (auto e : view) {
        HealthComponent& health = view.get<HealthComponent>(e);
        if (health.health <= 0) {
            registry.destroy(e);
        }
    }
}

void DefeatedPlayerSystem(entt::registry& registry) {
    auto view = registry.view<PlayerTag, HealthComponent>();
    auto weaponView = registry.view<WeaponTag>();
    for (auto e : view) {
        HealthComponent& health = view.get<HealthComponent>(e);
        if (health.health <= 0) {
            registry.destroy(e);
        }
    }
}

void AccumulatorSystems (entt::registry& registry, float delta_time) {

    //Despawn projectiles whose lifetime has expired
    auto projectileView = registry.view<ProjectileTag, LifetimeComponent>();
    for (auto e : projectileView) {
        float& remaining = projectileView.get<LifetimeComponent>(e).remaining;
        remaining -= delta_time;
        if (remaining <= 0.0f) {
            registry.destroy(e);
        }
    }

    // Handle player invulnerability frames
    auto playerView = registry.view<PlayerTag, IFramesComponent>();
    for (auto e : playerView) {
        IFramesComponent& iframes = playerView.get<IFramesComponent>(e);
        if (iframes.isInvuln) { 
            iframes.invulnAccumulator += delta_time;
            if (iframes.invulnAccumulator >= iframes.invulnTime) {
                iframes.isInvuln = false;
                iframes.invulnAccumulator = 0.0f;
            }
        }
    }

    // Handle hit flash effect
    auto hitFlashView = registry.view<HitFlashComponent, ColorComponent>();
    for (auto e : hitFlashView) {
        HitFlashComponent& hitFlash = hitFlashView.get<HitFlashComponent>(e);
        ColorComponent& colorComp = hitFlashView.get<ColorComponent>(e);
        if (hitFlash.isHit) {
            hitFlash.flashAccumulator += delta_time;
            colorComp.currentColor = hitFlash.flashColor;
            if (hitFlash.flashAccumulator >= hitFlash.flashDuration) {
                hitFlash.isHit = false;
                hitFlash.flashAccumulator = 0.0f;
                colorComp.currentColor = colorComp.originalColor;
            }
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

    entt::entity enemy_entity = registry.create();
    InitEnemy(registry, enemy_entity, Vector2{300.0f, 300.0f}, 50, 100.0f);

    entt::entity gun_entity = registry.create();
    InitGun(registry, gun_entity);

    while (!WindowShouldClose()) {
        float delta_time = GetFrameTime();


        PlayerInputSystem(registry);

        PlayerMovementSystem(registry, delta_time);
        EnemyMovementSystem(registry, delta_time);

        AimSystem(registry);
        FireSystem(registry, delta_time);
        HitSystem(registry);

        DefeatedEnemiesSystem(registry);
        AccumulatorSystems(registry, delta_time);

        DefeatedPlayerSystem(registry);

        BeginDrawing();
        ClearBackground(BLACK);
        DrawSystem(registry);
        EndDrawing();
    }

    UnloadResources(registry);
    CloseWindow();
    return 0;
}