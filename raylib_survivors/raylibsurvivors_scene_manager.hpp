#ifndef SCENE_MANAGER
#define SCENE_MANAGER

#include <raylib.h>

#include <iostream>
#include <string>
#include <unordered_map>

class SceneManager;

// Base class that all scenes inherit
class Scene {
    // Reference to the scene manager.
    // In practice, you would want to make this private (or protected)
    // and set this via the constructor.
    SceneManager* scene_manager;
public:
    // Begins the scene. This is where you can load resources
    virtual void Begin() = 0;

    // Ends the scene. This is where you can unload resources
    virtual void End() = 0;

    // Updates scene's state (physics, input, etc. can be added here)
    virtual void Update() = 0;

    // Draws the scene's current state
    virtual void Draw() = 0;

    void SetSceneManager(SceneManager* scene_manager) {
        this->scene_manager = scene_manager;
    }

    SceneManager* GetSceneManager() {
        return scene_manager;
    }
};


class SceneManager {
    // Mapping between a scene ID and a reference to the scene
    std::unordered_map<int, Scene*> scenes;

     // Current active scene
    Scene* active_scene = nullptr;

public:
    // Adds the specified scene to the scene manager, and assigns it
    // to the specified scene ID
    void RegisterScene(Scene* scene, int scene_id) {
        scenes[scene_id] = scene;
    }

    // Removes the scene identified by the specified scene ID
    // from the scene manager
    void UnregisterScene(int scene_id) {
        scenes.erase(scene_id);
    }

    // Switches to the scene identified by the specified scene ID.
    void SwitchScene(int scene_id) {
        // If the scene ID does not exist in our records,
        // don't do anything (or you can print an error message).
        if (scenes.find(scene_id) == scenes.end()) {
            std::cout << "Scene ID not found" << std::endl;
            return;
        }

        // If there is already an active scene, end it first
        if (active_scene != nullptr) {
            active_scene->End();
        }

        std::cout << "Moved to Scene " << scene_id << std::endl;

        active_scene = scenes[scene_id];

        active_scene->Begin();
    }

    // Gets the active scene
    Scene* GetActiveScene() {
        return active_scene;
    }
};

// Resource manager implemented as a singleton
class ResourceManager {
    std::unordered_map<std::string, Texture> textures;

    ResourceManager() {}

public:
    // Delete copy constructor and copy operator (=)
    // Ensures there will only be one instance of the resource manager
    ResourceManager(const ResourceManager&) = delete;
    void operator=(const ResourceManager&) = delete;

    static ResourceManager* GetInstance() {
        static ResourceManager insance;
        return &insance;
    }

    Texture GetTexture(const std::string& path) {
        // If the texture does not exist yet in our records,
        // load it and store it in memory.
        if (textures.find(path) == textures.end()) {
            std::cout << "Loaded " << path << " from Disk" << std::endl;
            textures[path] = LoadTexture(path.c_str());
        }
        else {
            std::cout << "Resource Already Loaded" << std::endl;
        }

        return textures[path];
    }

    // Used for unloading all the textures when the game is closed.
    void UnloadAllTextures() {
        for (auto it : textures) {
            UnloadTexture(it.second);
        }

        textures.clear();
    }
};

// Forward Declarations
extern const float FPS = 60.0f;
extern const float TIMESTEP = 1.0f / FPS; 
struct GridCell;
void InitGrid(std::vector<GridCell> &grid, int cellSize);
void AssignEntitiesToGrid(std::vector<GridCell> &grid, entt::registry& registry, int cellSize);

void InitPlayer(entt::registry& registry, entt::entity e, const Texture2D &playerTexture);
void InitEnemy(entt::registry& registry, entt::entity e, const Texture2D &enemyTexture, Vector2 position, float health, float moveSpeed);
void InitGun(entt::registry& registry, entt::entity e);
void InitXPOrb(entt::registry& registry, entt::entity e, Vector2 position, float xpAmount);
void InitChoices(entt::registry& registry);

void InitMainMenu(entt::registry& registry, SceneManager* sceneManager);
void InitLeaderboard(entt::registry& registry, SceneManager* sceneManager);
void UIRenderSystem(entt::registry& registry);

void InitNameInput(entt::registry& registry);
void LeaderboardRenderSystem(entt::registry& registry, SceneManager* sceneManager);
void NameInputSystem(entt::registry& registry, SceneManager* sceneManager, int score);
void NameInputRenderSystem(entt::registry& registry, int score);
void LeaderboardSwitchScene(entt::registry& registry, SceneManager* sceneManager);
void ClickHoverSystem(entt::registry& registry, SceneManager* sceneManager);

void PlayerInputSystem(entt::registry& registry);
void PlayerMovementSystem(entt::registry& registry, float delta_time);
void EnemyMovementSystem(entt::registry& registry, float delta_time);
void EnemySpawnSystem(entt::registry& registry, float delta_time, const Texture2D &enemyTexture, float timeElapsed);
void AimSystem(entt::registry& registry);
void FireSystem(entt::registry& registry, float delta_time);
void HitSystem(entt::registry& registry, float delta_time, std::vector<GridCell>& grid, int& score, bool& isPaused);
void DefeatedEnemiesSystem(entt::registry& registry, int& score);
void DefeatedPlayerSystem(entt::registry& registry, bool& isPaused, SceneManager* sceneManager, bool& isGameOver);
void AccumulatorSystems (entt::registry& registry, float delta_time);
bool IsPlayerLevelledUp(entt::registry& registry);
void RandomizeUpgrades(entt::registry& registry);
void ChooseUpgrade(entt::registry& registry, bool& isPaused);

void DrawSystem(entt::registry& registry, int score);

// ----- CREATING THE SCENES -----
// For the sake of not having so many headers, scenes will be created in this file.
// Ideally, you would have a separate file where you will define the scenes,
// and keep this file purely as a scene and resource manager.

class MainMenuScene : public Scene {
    // points to the registry in main loop
    entt::registry* registry;
    Texture2D backgroundTex{};

    public:
        MainMenuScene(entt::registry* r) : registry(r){}

        void Begin() override {
            backgroundTex = ResourceManager::GetInstance()->GetTexture("assets/MainMenuBG.png");
            InitMainMenu(*registry, GetSceneManager());
        }

        void End() override {
            registry->clear();
        }

        void Update() override {
            ClickHoverSystem(*registry, GetSceneManager());
        }

        void Draw() override {
            DrawTexture(backgroundTex, 0, 0, WHITE);
            UIRenderSystem(*registry);
        }
};

class GameScene : public Scene {
    // points to the registry in main loop
    entt::registry* registry;
    Texture2D playerTex{};
    Texture2D enemyTex{};

    std::vector<GridCell> grid;
    int cellSize = 80;
    int score = 0;
    bool isPaused;
    bool isGameOver = false;
    float timeElapsed = 0.0f;
    float accumulator = 0.0f;  

    public:
        GameScene(entt::registry* r) : registry(r){}

        void Begin() override {
            isGameOver = false;
            score = 0;
            // Initialize grid
            InitGrid(grid, cellSize);

            // Load textures
            playerTex = ResourceManager::GetInstance()->GetTexture("assets/player.png");
            enemyTex = ResourceManager::GetInstance()->GetTexture("assets/enemy.png");

            // Initialize entities
            entt::entity player_entity = registry->create();
            InitPlayer(*registry, player_entity, playerTex);

            entt::entity gun_entity = registry->create();
            InitGun(*registry, gun_entity);
            InitChoices(*registry);

            isPaused = false;

            timeElapsed = 0.0f;
            accumulator = 0.0f;
        }

        void End() override {
            registry->clear();
        }

        void Update() override {
            float delta_time = GetFrameTime();
            timeElapsed += delta_time;

            PlayerInputSystem(*registry);

            DefeatedPlayerSystem(*registry, isPaused, GetSceneManager(), isGameOver);

            if(!isPaused){
                accumulator += delta_time;  
                while (accumulator >= TIMESTEP) {
                    PlayerMovementSystem(*registry, TIMESTEP);
                    EnemyMovementSystem(*registry, TIMESTEP);

                    EnemySpawnSystem(*registry, TIMESTEP, enemyTex, timeElapsed);
                    AimSystem(*registry);
                    FireSystem(*registry, TIMESTEP);

                    AssignEntitiesToGrid(grid, *registry, cellSize);

                    HitSystem(*registry, TIMESTEP, grid, score, isPaused);

                    DefeatedEnemiesSystem(*registry, score);
                    AccumulatorSystems(*registry, TIMESTEP);

                    if (IsPlayerLevelledUp(*registry)) { 
                        RandomizeUpgrades(*registry);
                    }

                    accumulator -= TIMESTEP;
                } 
            } else {
                if (IsPlayerLevelledUp(*registry)) {
                    ChooseUpgrade(*registry, isPaused);
                }
            }
            if(isPaused) {
                NameInputSystem(*registry, GetSceneManager(), score);
                LeaderboardSwitchScene(*registry, GetSceneManager());
            }
        }



        void Draw() override {
            ClearBackground(BLACK);
            if (!isGameOver) {
                DrawSystem(*registry, score);
            }

            else {

                NameInputRenderSystem(*registry, score);
            }
        }
};

class LeaderboardScene : public Scene {
    entt::registry* registry;

    public: 
        LeaderboardScene(entt::registry* r) : registry(r){}
        
        void Begin() override {
            InitLeaderboard(*registry, GetSceneManager());
        }

        void End() override {
            registry->clear();
        }

        void Update() override {
            ClickHoverSystem(*registry, GetSceneManager());
        }

        void Draw() override {
            ClearBackground(BLACK);
            LeaderboardRenderSystem(*registry, GetSceneManager());
        }
};

#endif