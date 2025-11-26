#include <vector>
#include <raylib.h>
#include <raymath.h>
#include "entt.hpp"
#include <string>
#include <set>
#include <iostream>
// new
#include "raylibsurvivors_scene_manager.hpp"

const int WINDOW_WIDTH = 1280;
const int WINDOW_HEIGHT = 720;
// const float FPS = 60.0f;
// const float TIMESTEP = 1.0f / FPS; 

// ik these tags are not very ecs-y but to make things easier to manage HAHAHAH
struct PlayerTag {/* If an entity has this component, it is a player */};
struct EnemyTag {/* If an entity has this component, it is an enemy */};
struct WeaponTag { std::string name;/* If an entity has this component, it is a weapon */};
struct ProjectileTag { bool isPlayerProjectile;/* If an entity has this component, it is a projectile */};
struct XPOrbTag { float amount; /* If an entity has this component, it is an XP orb */};

enum class LevelUpOptions {
    NONE = 0,
    PLAYER_HEALTH = 1,
    PLAYER_MOVESPEED = 2,
    WEAPON_FIRERATE = 3,
    WEAPON_DAMAGE = 4,
    WEAPON_PIERCE = 5
};

struct ChoiceComponent { LevelUpOptions choice; Rectangle rect; std::string description; };

struct HealthComponent { float currentHealth; float maxHealth; };
struct IFramesComponent { bool isInvuln; float invulnTime; float invulnAccumulator; };
struct LevelComponent { int level; float experience; bool justLeveled = false; };

// Basic components for position, velocity, and movement
struct PositionComponent { Vector2 position; };
struct VelocityComponent { Vector2 velocity; };
struct MoveSpeedComponent { float speed; };
struct MassComponent { float mass; float inverseMass; };
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

struct ContactDamageComponent { float damage; float originalDamage; };

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

// Scene Manager
// enum class Scene {
//     MAIN_MENU,
//     GAME_PROPER,
//     // PAUSE_MENU,
//     // LEADERBOARD
// };

// UI components
struct SizeComponent { float width; float height; };
struct TextComponent { std::string label; };
struct HoverableComponent { bool isHovering; };
// check if entity is clicked then call function pointer
// source: https://en.cppreference.com/w/cpp/language/pointer.html#Pointers_to_functions
struct ClickableComponent { void (*onClick)(entt::registry&, SceneManager*); }; 

struct GridCell {
    std::vector<entt::entity> entities;
    int x, y;
    int numEntities;
    int width;
};

bool IsPlayerLevelledUp(entt::registry& registry);

void InitGrid(std::vector<GridCell> &grid, int cellSize) {
    grid.clear();
    const int numCols = (WINDOW_WIDTH + cellSize - 1) / cellSize;
    const int numRows = (WINDOW_HEIGHT + cellSize - 1) / cellSize;
    grid.reserve(numCols * numRows);

    for (int y = 0; y < WINDOW_HEIGHT; y += cellSize) {
        for (int x = 0; x < WINDOW_WIDTH; x += cellSize) {
            GridCell cell;
            cell.entities.clear();
            cell.numEntities = 0;
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
    
    auto view = registry.view<PositionComponent, VelocityComponent, CircleHitboxComponent>();
    for (auto e : view) {
        Vector2& pos = view.get<PositionComponent>(e).position;
        CircleHitboxComponent& hitbox = view.get<CircleHitboxComponent>(e);
        Vector2& vel = view.get<VelocityComponent>(e).velocity;

        Vector2 endPos = { pos.x + vel.x * TIMESTEP, pos.y + vel.y * TIMESTEP };

        Rectangle aabb = {
            std::min(pos.x, endPos.x) - hitbox.radius,
            std::min(pos.y, endPos.y) - hitbox.radius,
            std::abs(endPos.x - pos.x) + hitbox.radius * 2,
            std::abs(endPos.y - pos.y) + hitbox.radius * 2
        };

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

void InitPlayer(entt::registry& registry, entt::entity e, const Texture2D &playerTexture) {
    Vector2 center = {WINDOW_WIDTH / 2.0f, WINDOW_HEIGHT / 2.0f};
    float health = 100.0f;
    float moveSpeed = 200.0f;
    Vector2 spriteSize = {50, 50};
        
    registry.emplace<PlayerTag>(e);
    registry.emplace<IFramesComponent>(e, IFramesComponent{false, 0.5f, 0.0f});
    registry.emplace<CircleHitboxComponent>(e, CircleHitboxComponent{20.0f});
    registry.emplace<HealthComponent>(e, HealthComponent{health, health});
    registry.emplace<PositionComponent>(e, center);
    registry.emplace<VelocityComponent>(e, Vector2Zero());
    registry.emplace<AimDirectionComponent>(e, Vector2Zero());
    registry.emplace<ColorComponent>(e, WHITE);
    registry.emplace<MoveSpeedComponent>(e, moveSpeed);
    registry.emplace<SpriteComponent>(e, playerTexture, spriteSize);
    registry.emplace<HitFlashComponent>(e, false, 0.1f, 0.0f, RED);
    registry.emplace<LevelComponent>(e, LevelComponent{1, 0.0f, false});
}

void InitEnemy(entt::registry& registry, entt::entity e, 
                const Texture2D &enemyTexture,
                Vector2 position = {100.0f, 100.0f}, 
                float health = 10.0f, 
                float moveSpeed = 100.0f) {

    registry.emplace<EnemyTag>(e);
    registry.emplace<HealthComponent>(e, HealthComponent{health, health});
    registry.emplace<ContactDamageComponent>(e, 10.0f);
    registry.emplace<PositionComponent>(e, position);
    registry.emplace<VelocityComponent>(e, Vector2Zero());
    registry.emplace<MassComponent>(e, 1.0f, 1.0f);
    registry.emplace<AimDirectionComponent>(e, Vector2Zero());
    registry.emplace<ColorComponent>(e, YELLOW);
    registry.emplace<MoveSpeedComponent>(e, moveSpeed);
    registry.emplace<CircleHitboxComponent>(e, 20.0f);
    registry.emplace<HitFlashComponent>(e, false, 0.1f, 0.0f, RED);
    registry.emplace<SpriteComponent>(e, enemyTexture, Vector2{40, 40});
}

void InitGun (entt::registry& registry, entt::entity e) {
    registry.emplace<WeaponTag>(e, "Gun");
    registry.emplace<PositionComponent>(e, Vector2Zero());
    registry.emplace<AimDirectionComponent>(e, Vector2Zero());
    registry.emplace<FireRateComponent>(e, 1.0f, 0.0f); 
    registry.emplace<CircleHitboxComponent>(e, 5.0f);
    registry.emplace<ContactDamageComponent>(e, 10.0f);
    registry.emplace<MoveSpeedComponent>(e, 2000.0f);
    registry.emplace<LifetimeComponent>(e, 10.0f);
    registry.emplace<PierceComponent>(e, 1, 0);
    registry.emplace<MassComponent>(e, 0.0f, 0.0f);
}

void InitXPOrb(entt::registry& registry, entt::entity e, Vector2 position, float xpAmount) {
    registry.emplace<PositionComponent>(e, position);
    registry.emplace<CircleHitboxComponent>(e, 10.0f);
    registry.emplace<ColorComponent>(e, Color({0, 200, 255, 255}));
    registry.emplace<XPOrbTag>(e, xpAmount);
    registry.emplace<VelocityComponent>(e, Vector2Zero());
}

void InitChoices(entt::registry& registry) {
    entt::entity choice1 = registry.create();
    registry.emplace<ChoiceComponent>(choice1, LevelUpOptions::NONE, Rectangle{WINDOW_WIDTH / 2 - 350, WINDOW_HEIGHT / 2 - 100, 700, 50}, "");
    entt::entity choice2 = registry.create();
    registry.emplace<ChoiceComponent>(choice2, LevelUpOptions::NONE, Rectangle{WINDOW_WIDTH / 2 - 350, WINDOW_HEIGHT / 2, 700, 50}, "");
    entt::entity choice3 = registry.create();
    registry.emplace<ChoiceComponent>(choice3, LevelUpOptions::NONE, Rectangle{WINDOW_WIDTH / 2 - 350, WINDOW_HEIGHT / 2 + 100, 700, 50}, "" );
}

void PlayGameSystem(entt::registry& registry, SceneManager* sceneManager) {
    // Switch to Game Scene
    sceneManager->SwitchScene(1); 
}

void QuitGameSystem(entt::registry& registry, SceneManager* sceneManager) {
    CloseWindow();
}

void InitMainMenu(entt::registry& registry, SceneManager* sceneManager)
{
    // Background Texture
    entt::entity background = registry.create();
    Texture2D backgroundTex = ResourceManager::GetInstance()->GetTexture("assets/MainMenuBG.png");
    registry.emplace<SpriteComponent>(background, backgroundTex, Vector2{0.0f, 0.0f});

    // Play Button
    entt::entity playButton = registry. create();
    registry.emplace<PositionComponent>(playButton, Vector2{WINDOW_WIDTH/2 - 80, WINDOW_HEIGHT/2 + 100});
    registry.emplace<SizeComponent>(playButton, 150.0f, 50.0f);
    registry.emplace<TextComponent>(playButton, "Play");
    registry.emplace<HoverableComponent>(playButton, false);
    registry.emplace<ClickableComponent>(playButton, &PlayGameSystem);

    // Quit Button
    entt::entity quitButton = registry. create();
    registry.emplace<PositionComponent>(quitButton, Vector2{WINDOW_WIDTH/2 - 80, WINDOW_HEIGHT/2 + 200});
    registry.emplace<SizeComponent>(quitButton, 150.0f, 50.0f);
    registry.emplace<TextComponent>(quitButton, "Quit");
    registry.emplace<HoverableComponent>(quitButton, false);
    registry.emplace<ClickableComponent>(quitButton, &QuitGameSystem);
}

void UIRenderSystem(entt::registry& registry) {
    // Draw background
    auto bgView = registry.view<SpriteComponent>();
    for (auto e : bgView) {
        SpriteComponent& bgTexture = bgView.get<SpriteComponent>(e);
        DrawTexture(bgTexture.sprite, 0, 0, WHITE);
    }

    // Draw buttons
    auto view = registry.view<PositionComponent, SizeComponent, TextComponent, HoverableComponent>();
    for (auto e : view) {
        PositionComponent& position = view.get<PositionComponent>(e);
        SizeComponent& size = view.get<SizeComponent>(e);
        TextComponent& text = view.get<TextComponent>(e);
        HoverableComponent& hover = view.get<HoverableComponent>(e);
        Color buttonColor, textColor;

        if (hover.isHovering) {
            buttonColor = GREEN;
            textColor = WHITE;
        }
        else {
            buttonColor = WHITE;
            textColor = BLACK;
        }

        DrawRectangle(position.position.x, position.position.y, size.width, size.height, buttonColor);
        DrawText(text.label.c_str(), position.position.x + 50,  position.position.y + 12, 25, textColor);
    }
}

void ClickHoverSystem(entt::registry& registry, SceneManager* sceneManager) {
    Vector2 mousePosition = GetMousePosition();
    
    auto view = registry.view<PositionComponent, SizeComponent, HoverableComponent, ClickableComponent>();
    for (auto e : view) {
        PositionComponent& position = view.get<PositionComponent>(e);
        SizeComponent& size = view.get<SizeComponent>(e);
        ClickableComponent& click = view.get<ClickableComponent>(e);
        HoverableComponent& hover = view.get<HoverableComponent>(e);

        Rectangle button = { position.position.x, position.position.y, size.width, size.height };

        //new 
        hover.isHovering = CheckCollisionPointRec(mousePosition, button);

        if (hover.isHovering && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            if(click.onClick) {
                click.onClick(registry, sceneManager);
            }
        }
    }
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

void EnemySpawnSystem(entt::registry& registry, float delta_time, const Texture2D &enemyTexture, float timeElapsed) {
    static float spawnAccumulator = 0.0f;

    float baseSpawnRate = 0.5f;
    float baseSpawnGrowthPerSecond = 0.01f;   
    float enemiesPerSecond = baseSpawnRate + baseSpawnGrowthPerSecond * timeElapsed;
    float spawnInterval = 1.0f / enemiesPerSecond;

    float healthGrowthPerSecond = 0.5f;
    float moveSpeedGrowthPerSecond = 0.05f;
    float enemyHealth = 10.0f + healthGrowthPerSecond * timeElapsed;
    float enemyMoveSpeed = 25.0f + moveSpeedGrowthPerSecond * timeElapsed;

    spawnAccumulator += delta_time;
    while (spawnAccumulator >= spawnInterval) { 
        spawnAccumulator -= spawnInterval;

        entt::entity enemy_entity = registry.create();
        Vector2 spawnPosition;
        int edge = GetRandomValue(0, 3);
        switch (edge) {
            case 0: spawnPosition = { (float)GetRandomValue(0, WINDOW_WIDTH), 0.0f }; break;
            case 1: spawnPosition = { (float)GetRandomValue(0, WINDOW_WIDTH), (float)WINDOW_HEIGHT }; break;
            case 2: spawnPosition = { 0.0f, (float)GetRandomValue(0, WINDOW_HEIGHT) }; break;
            case 3: spawnPosition = { (float)WINDOW_WIDTH, (float)GetRandomValue(0, WINDOW_HEIGHT) }; break;
        }
        InitEnemy(registry, enemy_entity, enemyTexture, spawnPosition, enemyHealth, enemyMoveSpeed);
    }
}

void DrawSystem(entt::registry& registry, int score) {
    auto view = registry.view<PositionComponent, SpriteComponent, ColorComponent>();
    auto choiceView = registry.view<ChoiceComponent>();

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

        if (auto aim = registry.try_get<AimDirectionComponent>(e)) {
            angle = atan2f(aim->direction.y, aim->direction.x) * RAD2DEG;
        }

        DrawTexturePro(sprite, source, dest, origin, angle, color);
    }

    // Draw projectiles
    auto projectileView = registry.view<ProjectileTag, PositionComponent, CircleHitboxComponent, ColorComponent>();
    for (auto e : projectileView) {
        Vector2& pos = projectileView.get<PositionComponent>(e).position;
        CircleHitboxComponent& circle = projectileView.get<CircleHitboxComponent>(e);
        Color& color = projectileView.get<ColorComponent>(e).currentColor;
        DrawCircleV(pos, circle.radius, color);
    }

    auto healthView = registry.view<HealthComponent, PositionComponent, CircleHitboxComponent>();
    for (auto e : healthView) {
        HealthComponent& health = healthView.get<HealthComponent>(e);
        Vector2& pos = healthView.get<PositionComponent>(e).position;
        CircleHitboxComponent& circle = healthView.get<CircleHitboxComponent>(e);
        DrawRectangleV( Vector2{ pos.x - circle.radius, pos.y - circle.radius - 10 },
                        Vector2{ circle.radius * 2 * (health.currentHealth / health.maxHealth), 5 }, GREEN);
    }

    auto playerView = registry.view<PlayerTag, HealthComponent, LevelComponent>();
    for (auto e : playerView) {
        HealthComponent& health = playerView.get<HealthComponent>(e);
        LevelComponent& levelComp = playerView.get<LevelComponent>(e);
        DrawText(TextFormat("Health: %.0f/%.0f", health.currentHealth, health.maxHealth), 10, 40, 20, WHITE);
        DrawText(TextFormat("Level: %d", levelComp.level), 10, 70, 20, WHITE);
        DrawText(TextFormat("XP: %.0f / %.0f", levelComp.experience, levelComp.level * 100.0f), 10, 100, 20, WHITE);
    }

    // Draw Choices
    if (IsPlayerLevelledUp(registry)) {
            for (auto e : choiceView) {
            ChoiceComponent& choice = choiceView.get<ChoiceComponent>(e);
            Vector2 mousePos = GetMousePosition();
            Color rectColor = LIGHTGRAY;
            if (CheckCollisionPointRec(mousePos, choice.rect)) {
                rectColor = GRAY;
                if(IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
                    rectColor = YELLOW;
                }
            } else {
                rectColor = LIGHTGRAY;
            }
            DrawRectangleRec(choice.rect, rectColor);

            int fontSize = 20;
            int textWidth = MeasureText(choice.description.c_str(), fontSize);
            DrawText(choice.description.c_str(), choice.rect.x + (choice.rect.width - textWidth) / 2, choice.rect.y + (choice.rect.height - fontSize) / 2, fontSize, BLACK);
        }
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
    auto view = registry.view<  FireRateComponent, AimDirectionComponent, PositionComponent, 
                                CircleHitboxComponent, ContactDamageComponent, MoveSpeedComponent,
                                LifetimeComponent, PierceComponent, MassComponent>();
    for (auto e : view) {
        float& fireRate = view.get<FireRateComponent>(e).fireRate;
        float& fireRateAccumulator = view.get<FireRateComponent>(e).fireRateAccumulator;
        Vector2& aimDir = view.get<AimDirectionComponent>(e).direction;
        Vector2& position = view.get<PositionComponent>(e).position;

        CircleHitboxComponent& projHitbox = view.get<CircleHitboxComponent>(e);
        float& projDamage = view.get<ContactDamageComponent>(e).damage;
        float& projSpeed = view.get<MoveSpeedComponent>(e).speed;
        LifetimeComponent& projLifetime = view.get<LifetimeComponent>(e);
        PierceComponent& projPierce = view.get<PierceComponent>(e);

        fireRateAccumulator += delta_time;

        if (fireRateAccumulator >= fireRate) {
            fireRateAccumulator -= fireRate;

            entt::entity projectile = registry.create();

            float projectileSpeed = projSpeed;
            float projLifetimeRemaining = projLifetime.remaining;
            float projDamageValue = projDamage;

            Vector2 projectileVelocity = Vector2Scale(aimDir, projectileSpeed);
            Vector2 projectilePosition = position;

            registry.emplace<ProjectileTag>(projectile, true);
            registry.emplace<PositionComponent>(projectile, projectilePosition);
            registry.emplace<VelocityComponent>(projectile, projectileVelocity);
            registry.emplace<CircleHitboxComponent>(projectile, CircleHitboxComponent{projHitbox.radius});
            registry.emplace<MoveSpeedComponent>(projectile, MoveSpeedComponent{projectileSpeed});
            registry.emplace<ColorComponent>(projectile, ColorComponent{RED});
            registry.emplace<LifetimeComponent>(projectile, LifetimeComponent{ projLifetimeRemaining });
            registry.emplace<ContactDamageComponent>(projectile, projDamageValue);
            registry.emplace<PierceComponent>(projectile, projPierce);
            registry.emplace<MassComponent>(projectile, 0.0f, 0.0f);
        }
    }
}


void Collide(entt::registry &registry, entt::entity &a, entt::entity &b, float elasticity = 1.0f) {
    Vector2 aPos = registry.get<PositionComponent>(a).position;
    Vector2 aVel = registry.get<VelocityComponent>(a).velocity;
    float aInverseMass = registry.get<MassComponent>(a).inverseMass;
    Vector2 bPos = registry.get<PositionComponent>(b).position;
    Vector2 bVel = registry.get<VelocityComponent>(b).velocity;
    float bInverseMass = registry.get<MassComponent>(b).inverseMass;

    Vector2 normal = Vector2Subtract(bPos, aPos);
    Vector2 relativeVelocity = Vector2Subtract(bVel, aVel);
    float result = Vector2DotProduct(relativeVelocity, normal);
    if(result < 0) {
        Vector2 n = Vector2Normalize(normal);
        float r = Vector2DotProduct(relativeVelocity, n);

        float impulse = - (1 + elasticity) * r / ((Vector2Length(n)) * (aInverseMass + bInverseMass));
        bVel = Vector2Add(bVel, Vector2Scale(n, impulse * bInverseMass));
        aVel = Vector2Subtract(aVel, Vector2Scale(n, impulse * aInverseMass));
        registry.get<VelocityComponent>(b).velocity = bVel;
        registry.get<VelocityComponent>(a).velocity = aVel;
    }
}

bool SweptCollision(entt::registry &registry, entt::entity &a, entt::entity &b, float maxTime) {
    // taken from https://stackoverflow.com/questions/1073336/circle-line-segment-collision-detection-algorithm
    
    const Vector2 aPos = registry.get<PositionComponent>(a).position;
    const Vector2 aVel = registry.get<VelocityComponent>(a).velocity;
    const float aRadius = registry.get<CircleHitboxComponent>(a).radius;

    const Vector2 bPos = registry.get<PositionComponent>(b).position;
    const Vector2 bVel = registry.get<VelocityComponent>(b).velocity;
    const float bRadius = registry.get<CircleHitboxComponent>(b).radius;

    Vector2 relPos = Vector2Subtract(aPos, bPos);
    Vector2 relVel = Vector2Subtract(aVel, bVel); 

    float R = aRadius + bRadius;
    
    if (Vector2LengthSqr(relPos) <= R * R) return true;

    float a_coeff = Vector2DotProduct(relVel, relVel);
    if (a_coeff <= 0.0001f) return false;

    float b_coeff = 2.0f * Vector2DotProduct(relPos, relVel);
    float c_coeff = Vector2DotProduct(relPos, relPos) - R * R;

    float discriminant = b_coeff * b_coeff - 4.0f * a_coeff * c_coeff;
    if (discriminant < 0.0f) return false;

    float sqrtD = sqrtf(discriminant);
    float tEnter = (-b_coeff - sqrtD) / (2.0f * a_coeff);
    float tExit  = (-b_coeff + sqrtD) / (2.0f * a_coeff);

    if (tEnter >= 0.0f && tEnter <= maxTime) return true;
    return false;
}

void HitSystem(entt::registry& registry, float delta_time, std::vector<GridCell>& grid, int& score, bool& isPaused) {
    // types of collisions:
    // 1. projectile - enemy
    // 2. enemy - player
    // 3. player - xp orb

    auto projectileView = registry.view<ProjectileTag, PositionComponent, VelocityComponent, CircleHitboxComponent, ContactDamageComponent>();
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

                for (auto otherEntity : cell.entities) {
                    if (otherEntity != entity && enemyView.contains(otherEntity)) {
                        Vector2& enemyPos = enemyView.get<PositionComponent>(otherEntity).position;
                        Vector2& enemyVelocity = enemyView.get<VelocityComponent>(otherEntity).velocity;
                        CircleHitboxComponent& enemyHitbox = enemyView.get<CircleHitboxComponent>(otherEntity);
                        HealthComponent& enemyHealth = enemyView.get<HealthComponent>(otherEntity);
                        HitFlashComponent& enemyHitFlash = enemyView.get<HitFlashComponent>(otherEntity);
                        PierceComponent& pc = registry.get<PierceComponent>(entity);

                        //float dist = Vector2Distance(projPos, enemyPos);
                        bool hasPierced = std::find(pc.piercedEntities.begin(), pc.piercedEntities.end(), otherEntity) != pc.piercedEntities.end();
                        if (SweptCollision(registry, entity, otherEntity, delta_time) && !hasPierced) {
                            Collide(registry, entity, otherEntity);
                            enemyHealth.currentHealth -= projDamage;
                            enemyHitFlash.isHit = true;
                            
                            score += projDamage;
                            
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
                            playerHealth.currentHealth -= enemyDamage.damage;
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
                                levelComp.justLeveled = true;
                                isPaused = true; 
                            }

                            registry.destroy(otherEntity);
                        }
                    }
                }
            }
            
            if (enemyView.contains(entity)) {
                // prevent enemies from overlapping
                Vector2& enemyPos = enemyView.get<PositionComponent>(entity).position;
                CircleHitboxComponent& enemyHitbox = enemyView.get<CircleHitboxComponent>(entity);
                Vector2& enemyVelocity = enemyView.get<VelocityComponent>(entity).velocity;

                for (auto otherEntity : cell.entities) {
                    if (otherEntity != entity && enemyView.contains(otherEntity)) {
                        Vector2& otherEnemyPos = enemyView.get<PositionComponent>(otherEntity).position;
                        CircleHitboxComponent& otherEnemyHitbox = enemyView.get<CircleHitboxComponent>(otherEntity);

                        float dist = Vector2Distance(enemyPos, otherEnemyPos);
                        float minDist = enemyHitbox.radius + otherEnemyHitbox.radius;
                        if (dist < minDist && dist > 0.0f) {
                            Vector2 pushDir = Vector2Normalize(Vector2Subtract(enemyPos, otherEnemyPos));
                            float pushAmount = minDist - dist;
                            enemyPos = Vector2Add(enemyPos, Vector2Scale(pushDir, pushAmount * 0.5f));
                            otherEnemyPos = Vector2Subtract(otherEnemyPos, Vector2Scale(pushDir, pushAmount * 0.5f));
                        }
                    }
                }
            }
        }
    }
}

void DefeatedEnemiesSystem(entt::registry& registry, int& score) {
    auto view = registry.view<EnemyTag, HealthComponent, PositionComponent>();
    for (auto e : view) {
        HealthComponent& health = view.get<HealthComponent>(e);
        if (health.currentHealth <= 0) {
            score += 100;

            entt::entity xpOrb = registry.create();
            Vector2 position = view.get<PositionComponent>(e).position;
            InitXPOrb(registry, xpOrb, position, 50.0f);

            registry.destroy(e);
    
            
        }
    }
}

void DefeatedPlayerSystem(entt::registry& registry, bool& isPaused) {
    auto view = registry.view<PlayerTag, HealthComponent>();
    auto weaponView = registry.view<WeaponTag>();
    for (auto e : view) {
        HealthComponent& health = view.get<HealthComponent>(e);
        if (health.currentHealth <= 0) {
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


bool IsPlayerLevelledUp(entt::registry& registry) {
    auto view = registry.view<PlayerTag, LevelComponent>();
    for (auto e : view) {
        LevelComponent& levelComp = view.get<LevelComponent>(e);
        if (levelComp.justLeveled) {
            return true;
        }
    }
    return false;
}
void SetPlayerLevelledUp(entt::registry& registry, bool status) {
    auto view = registry.view<PlayerTag, LevelComponent>();
    for (auto e : view) {
        LevelComponent& levelComp = view.get<LevelComponent>(e);
        levelComp.justLeveled = status;
    }
}

void RandomizeUpgrades(entt::registry& registry) {
    auto choiceView = registry.view<ChoiceComponent>();
    for (auto e : choiceView) {
        ChoiceComponent& choice = choiceView.get<ChoiceComponent>(e);
        int option = GetRandomValue(1, 5);
        choice.choice = static_cast<LevelUpOptions>(option);
        switch (choice.choice) {
            case LevelUpOptions::PLAYER_HEALTH:
                choice.description = "Increase Max Health";
                break;
            case LevelUpOptions::PLAYER_MOVESPEED:
                choice.description = "Increase Move Speed";
                break;
            case LevelUpOptions::WEAPON_FIRERATE:
                choice.description = "Increase Weapon Fire Rate";
                break;
            case LevelUpOptions::WEAPON_DAMAGE:
                choice.description = "Increase Weapon Damage";
                break;
            case LevelUpOptions::WEAPON_PIERCE:
                choice.description = "Increase Weapon Pierce";
                break;
        }
    }
}

void ChooseUpgrade(entt::registry& registry, bool& isPaused) {
    auto choiceView = registry.view<ChoiceComponent>();
    auto playerView = registry.view<PlayerTag, HealthComponent, MoveSpeedComponent, LevelComponent>();
    auto weaponView = registry.view<WeaponTag, FireRateComponent, ContactDamageComponent, PierceComponent>();
    
    for (auto e : choiceView) {
        ChoiceComponent& choice = choiceView.get<ChoiceComponent>(e);
        if (CheckCollisionPointRec(GetMousePosition(), choice.rect) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            switch (choice.choice) {
                case LevelUpOptions::PLAYER_HEALTH: {
                    for (auto p : playerView) {
                        HealthComponent& health = playerView.get<HealthComponent>(p);
                        health.currentHealth += 20.0f;
                        health.maxHealth += 20.0f;
                    }
                    break;
                }
                case LevelUpOptions::PLAYER_MOVESPEED: {
                    for (auto p : playerView) {
                        MoveSpeedComponent& moveSpeed = playerView.get<MoveSpeedComponent>(p);
                        moveSpeed.speed += 20.0f;
                    }
                    break;
                }
                case LevelUpOptions::WEAPON_FIRERATE: {
                    for (auto w : weaponView) {
                        FireRateComponent& fireRate = weaponView.get<FireRateComponent>(w);
                        fireRate.fireRate -= 0.05f;
                    }
                    break;
                }
                case LevelUpOptions::WEAPON_DAMAGE: {
                    for (auto w : weaponView) {
                        ContactDamageComponent& damage = weaponView.get<ContactDamageComponent>(w);
                        damage.damage += 10;
                    }
                    break;
                }
                case LevelUpOptions::WEAPON_PIERCE: {
                    for (auto w : weaponView) {
                        PierceComponent& pierce = weaponView.get<PierceComponent>(w);
                        pierce.pierceCount += 1;
                    }
                    break;
                }
            }

            SetPlayerLevelledUp(registry, false);
            isPaused = false;
        }
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

    // Scene Manager
    SceneManager sceneManager;

    // Registering Scenes
    MainMenuScene mainMenuScene(&registry);
    mainMenuScene.SetSceneManager(&sceneManager);

    GameScene gameScene(&registry); 
    gameScene.SetSceneManager(&sceneManager);

    sceneManager.RegisterScene(&mainMenuScene, 0);
    sceneManager.RegisterScene(&gameScene, 1);

    // Start with main menu
    sceneManager.SwitchScene(0);

    while (!WindowShouldClose()) {
        Scene* activeScene = sceneManager.GetActiveScene();
        BeginDrawing();
        ClearBackground(WHITE);

        if (activeScene != nullptr) {
            activeScene->Update();
            activeScene->Draw();
        }
        EndDrawing();
    }

    Scene* activeScene = sceneManager.GetActiveScene();
    if (activeScene != nullptr) {
        activeScene->End();
    }

    ResourceManager::GetInstance()->UnloadAllTextures();

    CloseWindow();
    return 0;
}
