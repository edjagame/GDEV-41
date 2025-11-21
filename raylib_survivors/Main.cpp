#include <vector>
#include <raylib.h>
#include <raymath.h>
#include "entt.hpp"

const int WINDOW_WIDTH = 1280;
const int WINDOW_HEIGHT = 720;
const float FPS = 60.0f;
const float TIMESTEP = 1 / FPS; 

// ik these tags are not very ecs-y but to make things easier to manage HAHAHAH
struct PlayerTag {/* If an entity has this component, it is a player */};
struct EnemyTag {/* If an entity has this component, it is an enemy */};
struct WeaponTag {/* If an entity has this component, it is a weapon */};
struct ProjectileTag { bool isPlayerProjectile;/* If an entity has this component, it is a projectile */};
struct XPOrbTag { float amount; /* If an entity has this component, it is an XP orb */};

struct HealthComponent { float health; };
struct IFramesComponent { bool isInvuln; float invulnTime; float invulnAccumulator; };
struct LevelComponent { int level; float experience; };

// Basic components for position, velocity, and movement
struct PositionComponent { Vector2 position; };
struct VelocityComponent { Vector2 velocity; };
struct MoveSpeedComponent { float speed; };

// Shape components for drawing
struct RectangleHitboxComponent { Vector2 size; };
struct CircleHitboxComponent { 
    float radius; 
    Rectangle GetAABB(const Vector2& position) const {
        return Rectangle{
            position.x - radius,
            position.y - radius,
            radius * 2,
            radius * 2
        };
    }
};

struct ContactDamageComponent { float damage; };

// Drawing related components
struct SpriteComponent { Texture2D sprite; Vector2 spriteSize; };
struct ColorComponent { Color originalColor; Color currentColor = originalColor; };
struct HitFlashComponent { bool isHit; float flashDuration; float flashAccumulator; Color flashColor; };

// Weapon data components
struct AimDirectionComponent { Vector2 direction; };
struct FireRateComponent { float fireRate; float fireRateAccumulator; };
struct LifetimeComponent { float remaining; };
struct PierceComponent { int pierceCount; int pierceAccumulator; std::vector<entt::entity> piercedEntities; };
struct KnockbackComponent { float force; };

// Projectile data for spawning projectiles (used by weapons)
struct ProjectileComponent {    CircleHitboxComponent projectileHitbox; 
                                ContactDamageComponent projectileDamage;
                                MoveSpeedComponent projectileSpeed; 
                                LifetimeComponent projectileLifetime;
                                PierceComponent projectilePierce;
                                KnockbackComponent projectileKnockback;
                            };

struct GridCell {
    std::vector<entt::entity> entities;
    int x, y;
    int numEntities;
    int width;
};

void InitGrid(std::vector<GridCell> &grid, int cellSize) {
    for(GridCell& cell : grid) {
        cell.entities.clear();
        cell.numEntities = 0;
    }

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

void AssignEntitiesToGrid(std::vector<GridCell> &grid, entt::registry& registry, int cellSize) {

    float cellWidth = (float)cellSize;

    for (auto &cell : grid) {
        cell.entities.clear();
        cell.numEntities = 0;
    }

    const int numCols = (WINDOW_WIDTH + cellSize - 1) / cellSize;
    const int numRows = (WINDOW_HEIGHT + cellSize - 1) / cellSize;
    
    auto view = registry.view<PositionComponent, CircleHitboxComponent>();
    for (auto e : view) {
        Vector2& pos = view.get<PositionComponent>(e).position;
        CircleHitboxComponent& hitbox = view.get<CircleHitboxComponent>(e);

        Rectangle aabb = hitbox.GetAABB(pos);

        int leftCol = std::max(0, (int)(aabb.x / cellWidth));
        int rightCol = std::min(numCols - 1, (int)((aabb.x + aabb.width) / cellWidth));
        int topRow = std::max(0, (int)(aabb.y / cellWidth));
        int bottomRow = std::min(numRows - 1, (int)((aabb.y + aabb.height) / cellWidth));

        for (int i = leftCol; i<= rightCol; i++) {
            for (int j = topRow; j<= bottomRow; j++) {
                int cellIndex = j * numCols + i;
                grid[cellIndex].entities.push_back(e);
                grid[cellIndex].numEntities++;
            }
        }
    }
}

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
    registry.emplace<AimDirectionComponent>(e, Vector2Zero());
    registry.emplace<ColorComponent>(e, WHITE);
    registry.emplace<MoveSpeedComponent>(e, moveSpeed);
    registry.emplace<SpriteComponent>(e, sprite, spriteSize);
    registry.emplace<HitFlashComponent>(e, false, 0.1f, 0.0f, RED);
    registry.emplace<LevelComponent>(e, LevelComponent{1, 0.0f});
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
    
    registry.emplace<SpriteComponent>(e, LoadTexture("assets/enemy.png"), Vector2{40, 40});
}

void InitGun (entt::registry& registry, entt::entity e) {
    registry.emplace<WeaponTag>(e);
    registry.emplace<PositionComponent>(e, Vector2Zero());
    registry.emplace<AimDirectionComponent>(e, Vector2Zero());
    registry.emplace<FireRateComponent>(e, 0.3f, 0.0f); 
    registry.emplace<ProjectileComponent>(  e, 
                                            CircleHitboxComponent{5.0f}, 
                                            10.0f,
                                            MoveSpeedComponent{1000.0f}, 
                                            10.0f,
                                            PierceComponent{3, 0},
                                            KnockbackComponent{500.0f} );
}

void InitXPOrb(entt::registry& registry, entt::entity e, Vector2 position, float xpAmount) {
    registry.emplace<PositionComponent>(e, position);
    registry.emplace<CircleHitboxComponent>(e, CircleHitboxComponent{10.0f});
    registry.emplace<ColorComponent>(e, GREEN);
    registry.emplace<XPOrbTag>(e, xpAmount);
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

    // Clamp player position
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
    auto enemyView = registry.view<EnemyTag, PositionComponent, VelocityComponent, MoveSpeedComponent, AimDirectionComponent>();
    auto playerView = registry.view<PlayerTag, PositionComponent>();

    // Get player position
    Vector2 playerPos = Vector2Zero();
    for (auto p : playerView) {
        playerPos = playerView.get<PositionComponent>(p).position;
    }

    for (auto e : enemyView) {
        Vector2& enemyPos = enemyView.get<PositionComponent>(e).position;
        Vector2& enemyVelocity = enemyView.get<VelocityComponent>(e).velocity;
        float& speed = enemyView.get<MoveSpeedComponent>(e).speed;
        Vector2& aimDir = enemyView.get<AimDirectionComponent>(e).direction;

        aimDir = Vector2Normalize(Vector2Subtract(playerPos, enemyPos));
        Vector2 velocity = Vector2Scale(aimDir, speed);

        enemyVelocity = velocity;

    }
}

void EnemySpawnSystem(entt::registry& registry, float delta_time) {
    static float spawnAccumulator = 0.0f;
    static float spawnInterval = 0.5f;

    spawnAccumulator += delta_time;
    if (spawnAccumulator >= spawnInterval) {
        spawnAccumulator = 0.0f;

        entt::entity enemy_entity = registry.create();
        Vector2 spawnPosition;
        int edge = GetRandomValue(0, 3);
        switch (edge) {
            case 0: 
                spawnPosition = { (float)GetRandomValue(0, WINDOW_WIDTH), 0.0f };
                break;
            case 1: 
                spawnPosition = { (float)GetRandomValue(0, WINDOW_WIDTH), (float)WINDOW_HEIGHT };
                break;
            case 2: 
                spawnPosition = { 0.0f, (float)GetRandomValue(0, WINDOW_HEIGHT) };
                break;
            default:
                spawnPosition = { (float)WINDOW_WIDTH, (float)GetRandomValue(0, WINDOW_HEIGHT) };
                break;
        }
        InitEnemy(registry, enemy_entity, spawnPosition, 50, 100.0f);
    }
}

void DrawSystem(entt::registry& registry, int score) {
    auto view = registry.view<PositionComponent, SpriteComponent, ColorComponent>();

    // Draw xp orbs TEMP
    auto xpOrbView = registry.view<XPOrbTag, PositionComponent, CircleHitboxComponent, ColorComponent>();
    for (auto e : xpOrbView) {
        Vector2& pos = xpOrbView.get<PositionComponent>(e).position;
        CircleHitboxComponent& circle = xpOrbView.get<CircleHitboxComponent>(e);
        Color& color = xpOrbView.get<ColorComponent>(e).currentColor;
        DrawCircleV(pos, circle.radius, color);
    }
    
    for (auto e : view) {
        Vector2& pos = view.get<PositionComponent>(e).position;
        Texture2D& sprite = view.get<SpriteComponent>(e).sprite;
        Vector2& spriteSize = view.get<SpriteComponent>(e).spriteSize;
        Color& color = view.get<ColorComponent>(e).currentColor;

        Rectangle source = { 0.0f, 0.0f, (float)sprite.width, (float)sprite.height };
        Rectangle dest = { pos.x, pos.y, spriteSize.x, spriteSize.y };
        Vector2 origin = { spriteSize.x * 0.5f, spriteSize.y * 0.5f };

        float angle = 0.0f;
        // If entity has an AimDirectionComponent, use it to compute rotation
        if (auto aim = registry.try_get<AimDirectionComponent>(e)) {
            angle = atan2f(aim->direction.y, aim->direction.x) * RAD2DEG;
        }

        DrawTexturePro(sprite, source, dest, origin, angle, color);
    }

    //Draw Hitboxes
    // auto shapeView = registry.view<PositionComponent, CircleHitboxComponent, ColorComponent>();
    // for (auto e : shapeView) {
    //     Vector2& pos = shapeView.get<PositionComponent>(e).position;
    //     CircleHitboxComponent& circle = shapeView.get<CircleHitboxComponent>(e);
    //     Color& color = shapeView.get<ColorComponent>(e).currentColor;

    //     DrawCircleV(pos, circle.radius, color);
    // }

    // Draw projectiles
    auto projectileView = registry.view<ProjectileTag, PositionComponent, CircleHitboxComponent, ColorComponent>();
    for (auto e : projectileView) {
        Vector2& pos = projectileView.get<PositionComponent>(e).position;
        CircleHitboxComponent& circle = projectileView.get<CircleHitboxComponent>(e);
        Color& color = projectileView.get<ColorComponent>(e).currentColor;
        DrawCircleV(pos, circle.radius, color);
    }

    auto playerView = registry.view<PlayerTag, HealthComponent, LevelComponent>();
    for (auto e : playerView) {
        HealthComponent& health = playerView.get<HealthComponent>(e);
        LevelComponent& levelComp = playerView.get<LevelComponent>(e);
        DrawText(TextFormat("Health: %.0f", health.health), 10, 40, 20, WHITE);
        DrawText(TextFormat("Level: %d", levelComp.level), 10, 70, 20, WHITE);
        DrawText(TextFormat("XP: %.0f / %.0f", levelComp.experience, levelComp.level * 100.0f), 10, 100, 20, WHITE);
    }

    // Draw Score
    DrawText(TextFormat("Score: %d", score), 10, 10, 20, WHITE);
}


// system that aims from player to mouse position
// note: needs to be called after InitPlayer
void AimSystem(entt::registry& registry) {
    
    // Get player position
    Vector2 playerPos = Vector2Zero();
    auto playerView = registry.view<PlayerTag, PositionComponent>();
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

    auto playerAimView = registry.view<PlayerTag, AimDirectionComponent, PositionComponent>();
    for (auto e : playerAimView) {
        Vector2& aimDir = playerAimView.get<AimDirectionComponent>(e).direction;
        Vector2& position = playerAimView.get<PositionComponent>(e).position;

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
            fireRateAccumulator -= fireRate;

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
            registry.emplace<PierceComponent>(projectile, projectileData.projectilePierce);
            registry.emplace<KnockbackComponent>(projectile, projectileData.projectileKnockback);
        }
    }
}

void HitSystem(entt::registry& registry, float delta_time, std::vector<GridCell>& grid, int& score) {
    // types of collisions:
    // 1. projectile - enemy
    // 2. enemy - player
    // 3. player - xp orb

    auto projectileView = registry.view<ProjectileTag, PositionComponent, VelocityComponent, CircleHitboxComponent, ContactDamageComponent, KnockbackComponent>();
    auto enemyView = registry.view<EnemyTag, PositionComponent, VelocityComponent, CircleHitboxComponent, HealthComponent, ContactDamageComponent, HitFlashComponent>();
    auto playerView = registry.view<PlayerTag, PositionComponent, CircleHitboxComponent, HealthComponent, IFramesComponent, HitFlashComponent>();
    auto xpOrbView = registry.view<XPOrbTag, PositionComponent, CircleHitboxComponent>();

    for (auto& cell : grid) {
        for (auto entity : cell.entities) {
            // 1. Projectile-Enemy collisions
            if (projectileView.contains(entity)) {
                Vector2& projPos = projectileView.get<PositionComponent>(entity).position;
                CircleHitboxComponent& projHitbox = projectileView.get<CircleHitboxComponent>(entity);
                float projDamage = projectileView.get<ContactDamageComponent>(entity).damage;
                Vector2& projVelocity = projectileView.get<VelocityComponent>(entity).velocity;
                KnockbackComponent& projKnockback = projectileView.get<KnockbackComponent>(entity);

                for (auto otherEntity : cell.entities) {
                    if (otherEntity != entity && enemyView.contains(otherEntity)) {
                        Vector2& enemyPos = enemyView.get<PositionComponent>(otherEntity).position;
                        Vector2& enemyVelocity = enemyView.get<VelocityComponent>(otherEntity).velocity;
                        CircleHitboxComponent& enemyHitbox = enemyView.get<CircleHitboxComponent>(otherEntity);
                        HealthComponent& enemyHealth = enemyView.get<HealthComponent>(otherEntity);
                        HitFlashComponent& enemyHitFlash = enemyView.get<HitFlashComponent>(otherEntity);
                        PierceComponent& pc = registry.get<PierceComponent>(entity);

                        float dist = Vector2Distance(projPos, enemyPos);
                        bool hasPierced = std::find(pc.piercedEntities.begin(), pc.piercedEntities.end(), otherEntity) != pc.piercedEntities.end();
                        if (dist <= (projHitbox.radius + enemyHitbox.radius) && !hasPierced) {
                            enemyHealth.health -= projDamage;
                            enemyHitFlash.isHit = true;
                            
                            score += 10;
                            
                            // Knockback effect
                            enemyVelocity = Vector2Add(enemyVelocity, Vector2Scale(Vector2Normalize(projVelocity), projKnockback.force));

                            pc.pierceAccumulator += 1;
                            pc.piercedEntities.push_back(otherEntity);
                            if (pc.pierceAccumulator >= pc.pierceCount) {
                                registry.destroy(entity);
                                break;
                            }
                        }
                    }
                }
            }

            if (playerView.contains(entity)) {
                Vector2& playerPos = playerView.get<PositionComponent>(entity).position;
                CircleHitboxComponent& playerHitbox = playerView.get<CircleHitboxComponent>(entity);
                HealthComponent& playerHealth = playerView.get<HealthComponent>(entity);
                IFramesComponent& playerIFrames = playerView.get<IFramesComponent>(entity);
                HitFlashComponent& playerHitFlash = playerView.get<HitFlashComponent>(entity);

                // 2. Enemy-Player collisions
                for (auto otherEntity : cell.entities) {
                    if (otherEntity != entity && enemyView.contains(otherEntity)) {
                        Vector2& enemyPos = enemyView.get<PositionComponent>(otherEntity).position;
                        CircleHitboxComponent& enemyHitbox = enemyView.get<CircleHitboxComponent>(otherEntity);
                        ContactDamageComponent& enemyDamage = enemyView.get<ContactDamageComponent>(otherEntity);

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
                
                // 3. Player-XPOrb collisions
                for (auto otherEntity : cell.entities) {
                    if (otherEntity != entity && xpOrbView.contains(otherEntity)) {
                        Vector2& orbPos = xpOrbView.get<PositionComponent>(otherEntity).position;
                        CircleHitboxComponent& orbHitbox = xpOrbView.get<CircleHitboxComponent>(otherEntity);
                        XPOrbTag& xpOrbTag = xpOrbView.get<XPOrbTag>(otherEntity);

                        float dist = Vector2Distance(playerPos, orbPos);
                        if (dist <= (playerHitbox.radius + orbHitbox.radius)) {

                            LevelComponent& levelComp = registry.get<LevelComponent>(entity);
                            levelComp.experience += xpOrbTag.amount;

                            float xpForNextLevel = levelComp.level * 100.0f;
                            if (levelComp.experience >= xpForNextLevel) {
                                levelComp.level += 1;
                                levelComp.experience -= xpForNextLevel;
                            }

                            registry.destroy(otherEntity);
                        }
                    }
                }
            }
        }
    }
}

//     // Get player position
//     Vector2 playerPos = Vector2Zero();
//     for (auto e : playerView) {
//         playerPos = playerView.get<PositionComponent>(e).position;
//     }

//     // Projectile-Enemy collisions
//     for (auto proj : projectileView) {
//         Vector2& projPos = projectileView.get<PositionComponent>(proj).position;
//         CircleHitboxComponent& projHitbox = projectileView.get<CircleHitboxComponent>(proj);
//         float projDamage = projectileView.get<ContactDamageComponent>(proj).damage;
//         Vector2& projVelocity = projectileView.get<VelocityComponent>(proj).velocity;
//         KnockbackComponent& projKnockback = projectileView.get<KnockbackComponent>(proj);

//         for (auto enemy : enemyView) {
//             Vector2& enemyPos = enemyView.get<PositionComponent>(enemy).position;
//             Vector2& enemyVelocity = enemyView.get<VelocityComponent>(enemy).velocity;
//             CircleHitboxComponent& enemyHitbox = enemyView.get<CircleHitboxComponent>(enemy);
//             HealthComponent& enemyHealth = enemyView.get<HealthComponent>(enemy);
//             HitFlashComponent& enemyHitFlash = enemyView.get<HitFlashComponent>(enemy);
//             PierceComponent& pc = registry.get<PierceComponent>(proj);

//             float dist = Vector2Distance(projPos, enemyPos);
//             bool hasPierced = std::find(pc.piercedEntities.begin(), pc.piercedEntities.end(), enemy) != pc.piercedEntities.end();
//             if (dist <= (projHitbox.radius + enemyHitbox.radius) && !hasPierced) {
//                 enemyHealth.health -= projDamage;
//                 enemyHitFlash.isHit = true;
                
//                 score += 10;
                
//                 // Knockback effect
//                 enemyVelocity = Vector2Add(enemyVelocity, Vector2Scale(Vector2Normalize(projVelocity), projKnockback.force));

//                 pc.pierceAccumulator += 1;
//                 pc.piercedEntities.push_back(enemy);
//                 if (pc.pierceAccumulator >= pc.pierceCount) {
//                     registry.destroy(proj);
//                     break;
//                 }
//             }
//         }
//     }

//     // Enemy-Player collisions
//     for (auto player : playerView) {
//         Vector2& playerPos = playerView.get<PositionComponent>(player).position;
//         CircleHitboxComponent& playerHitbox = playerView.get<CircleHitboxComponent>(player);
//         HealthComponent& playerHealth = playerView.get<HealthComponent>(player);
//         IFramesComponent& playerIFrames = playerView.get<IFramesComponent>(player);
//         HitFlashComponent& playerHitFlash = playerView.get<HitFlashComponent>(player);

//         // Enemy-Player collisions
//         for (auto enemy : enemyView) {
//             Vector2& enemyPos = enemyView.get<PositionComponent>(enemy).position;
//             CircleHitboxComponent& enemyHitbox = enemyView.get<CircleHitboxComponent>(enemy);
//             ContactDamageComponent& enemyDamage = enemyView.get<ContactDamageComponent>(enemy);

//             float dist = Vector2Distance(playerPos, enemyPos);
//             if (dist <= (playerHitbox.radius + enemyHitbox.radius) && !playerIFrames.isInvuln) {
//                 playerHealth.health -= enemyDamage.damage;
//                 playerHitFlash.isHit = true;
//                 playerIFrames.isInvuln = true;
//                 playerIFrames.invulnAccumulator = 0.0f;
//                 break;
//             }
//         }

//         for (auto xpOrb : xpOrbView) {
//             Vector2& orbPos = xpOrbView.get<PositionComponent>(xpOrb).position;
//             CircleHitboxComponent& orbHitbox = xpOrbView.get<CircleHitboxComponent>(xpOrb);
//             XPOrbTag& xpOrbTag = xpOrbView.get<XPOrbTag>(xpOrb);

//             float dist = Vector2Distance(playerPos, orbPos);
//             if (dist <= (playerHitbox.radius + orbHitbox.radius)) {

//                 LevelComponent& levelComp = registry.get<LevelComponent>(player);
//                 levelComp.experience += xpOrbTag.amount;

//                 float xpForNextLevel = levelComp.level * 100.0f;
//                 if (levelComp.experience >= xpForNextLevel) {
//                     levelComp.level += 1;
//                     levelComp.experience -= xpForNextLevel;
//                 }

//                 registry.destroy(xpOrb);
//             }
//         }
//     }
// }

void DefeatedEnemiesSystem(entt::registry& registry, int& score) {
    auto view = registry.view<EnemyTag, HealthComponent, PositionComponent>();
    for (auto e : view) {
        HealthComponent& health = view.get<HealthComponent>(e);
        if (health.health <= 0) {
            score += 100;

            entt::entity xpOrb = registry.create();
            Vector2 position = view.get<PositionComponent>(e).position;
            InitXPOrb(registry, xpOrb, position, 20.0f);

            registry.destroy(e);

            
        }
    }
}

void DefeatedPlayerSystem(entt::registry& registry, bool& isPaused) {
    auto view = registry.view<PlayerTag, HealthComponent>();
    auto weaponView = registry.view<WeaponTag>();
    for (auto e : view) {
        HealthComponent& health = view.get<HealthComponent>(e);
        if (health.health <= 0) {
            registry.destroy(e);
            isPaused = true;
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

    std::vector<GridCell> grid;
    InitGrid(grid, 80);

    entt::registry registry;
    entt::entity player_entity = registry.create();
    InitPlayer(registry, player_entity);

    entt::entity gun_entity = registry.create();
    InitGun(registry, gun_entity);


    int score = 0;
    bool isPaused = false;

    while (!WindowShouldClose()) {
        float delta_time = GetFrameTime();

        if (!isPaused) {
            AssignEntitiesToGrid(grid, registry, 80);

            PlayerInputSystem(registry);
            PlayerMovementSystem(registry, delta_time);
            EnemyMovementSystem(registry, delta_time);

            EnemySpawnSystem(registry, delta_time);

            AimSystem(registry);
            FireSystem(registry, delta_time);
            HitSystem(registry, delta_time, grid, score);

            DefeatedEnemiesSystem(registry, score);
            AccumulatorSystems(registry, delta_time);

            DefeatedPlayerSystem(registry, isPaused);
        }

        BeginDrawing();
        ClearBackground(BLACK);
        DrawSystem(registry, score);

        // draw grid cells if it contains entities
        // for (const auto& cell : grid) {
        //     if (cell.numEntities > 0) {
        //         DrawRectangleLines(cell.x * cell.width, cell.y * cell.width, cell.width, cell.width, WHITE);
        //     }
        // }
        EndDrawing();
    }

    UnloadResources(registry);
    CloseWindow();
    return 0;
}