// Dodge: move left and right to avoid the falling blocks. Survive as long as you can.
//
// Controls: A / D or Left / Right to move. R to restart after you are hit.
//
// Shows: generated textures, the camera / light / background setup every game needs,
// input actions (hold and press), custom components, systems, spawning and destroying
// entities, a cached query for collision checks, and a HUD drawn with an editor tool.
// Uses no files: every texture is generated in memory.

#include <Module.hpp>
#include <Log/Module.hpp>
#include <Input/Module.hpp>
#include <Editor/Module.hpp>
#include <Filesystem/Module.hpp>
#include <Types/Graphics.hpp>
#include <Gameplay/ECS/Module.hpp>
#include <Gameplay/ECS/Modules/TransformModule.hpp>
#include <Gameplay/ECS/Modules/GraphicsModule.hpp>
#include <Rendering/Components/RenderingComponent.hpp>

#include <imgui.h>
#include <algorithm>
#include <cstdlib>
#include <random>

// The engine's components (FTransformComponent, FTexture2DComponent, ...).
using namespace Components;

DECLARE_LOGGER(Dodge, debug)

//! [actions]
// ---- Input actions ----------------------------------------------------------
DEFINE_INPUT_ACTION(MoveX, float)       // -1 = left, +1 = right
DEFINE_INPUT_ACTION(Restart, bool)
//! [actions]

//! [components]
// ---- Components -------------------------------------------------------------
// DEFINE_COMPONENT must be used inside namespace Components.
namespace Components {
    DEFINE_COMPONENT(FPlayer)
    DEFINE_COMPONENT(FFalling,
        ECS_MEMBER(float, Speed, 60.f))
    DEFINE_COMPONENT(FSpawner,
        ECS_MEMBER(float, Timer, 0.f))
}
//! [components]

// ---- Game state -------------------------------------------------------------
constexpr float ScreenW = 320.f;
constexpr float ScreenH = 180.f;
constexpr float PlayerSize = 14.f;

struct SDodgeState {
    float Survived = 0.f;   // seconds survived this round
    float Best = 0.f;
    bool bGameOver = false;
};
static SDodgeState Game;
static std::mt19937 Rng { std::random_device{}() };

static float RandomRange(float Min, float Max) {
    return std::uniform_real_distribution<float>(Min, Max)(Rng);
}

//! [scene_helpers]
// ---- Scene helpers (the same in every example) -------------------------------
// Creates a square, solid-white texture in memory and registers it with the
// Filesystem module under AssetPath, as if it were an image file in Resources/.
static void RegisterSolidTexture(const std::string& AssetPath, int Size) {
    SDL_Surface* surface = SDL_CreateSurface(Size, Size, SDL_PIXELFORMAT_ABGR8888);
    const Uint32 white = SDL_MapSurfaceRGBA(surface, 255, 255, 255, 255);
    SDL_FillSurfaceRect(surface, nullptr, white);

    TObjectPtr<Graphics::STexture2D> texture;
    texture->Surface = surface;
    texture->Width = static_cast<float>(Size);
    texture->Height = static_cast<float>(Size);
    texture->AssetPath = AssetPath;

    TObjectPtr<Filesystem::SAssetData> assetData;
    assetData->AssetPath = AssetPath;
    assetData->AssetName = AssetPath;
    assetData->AssetType = Filesystem::TEXTURE;
    assetData->ObjectData = texture;
    Filesystem::RegisterAssetData(assetData);
}

// A fixed camera showing (0,0)-(320,180), a light covering the whole view,
// and a full-screen sprite on the Background layer that clears the screen.
static void CreateCameraAndBackground(const flecs::world& World, SDL_FColor Color) {
    World.entity("Camera")
        .set<FTransformComponent>({})
        .set<FCameraComponent>({
            .CameraSize = {ScreenW, ScreenH},
            .Boundaries = {0, 0, 0, 0},
        })
        .set<FLightComponent>({ .Radius = 100.f, .Intensity = 2.4f });

    World.entity("Background")
        .set<FTransformComponent>({})
        .set<FTexture2DComponent>({
            .Sprite = { .SpriteSheet = {"White"} },
            .TextureSize = {ScreenW, ScreenH},
            .Color = Color,
        })
        .set<FRenderingComponent>({ .RenderLayer = Renderer::Background });
}

// A coloured rectangle on the Entities layer. Its top-left corner is at Position.
static flecs::entity SpawnRect(const flecs::world& World, glm::vec2 Position,
                               glm::vec2 Size, SDL_FColor Color) {
    return World.entity()
        .set<FTransformComponent>({ .Location = {Position.x, Position.y, 0.f} })
        .set<FTexture2DComponent>({
            .Sprite = { .SpriteSheet = {"White"} },
            .TextureSize = Size,
            .Color = Color,
        })
        .set<FRenderingComponent>({ .RenderLayer = Renderer::Entities });
}

static bool Overlaps(glm::vec2 PosA, glm::vec2 SizeA, glm::vec2 PosB, glm::vec2 SizeB) {
    return PosA.x < PosB.x + SizeB.x && PosB.x < PosA.x + SizeA.x &&
           PosA.y < PosB.y + SizeB.y && PosB.y < PosA.y + SizeA.y;
}
//! [scene_helpers]

//! [hud]
// ---- HUD ----------------------------------------------------------------------
// Editor tools are drawn every frame between ImGui::NewFrame() and the render, so
// they are the place for game UI too. Open it once with CoreEditor::Open<DodgeHud>().
class DodgeHud : public ImGUIWrapper::IEditorTool {
protected:
    void Draw_Internal() override {
        // The engine always shows the Dear ImGui demo window.
        // Collapse it once so it doesn't cover the game.
        ImGui::SetWindowCollapsed("Dear ImGui Demo", true, ImGuiCond_Once);

        const ImGuiViewport* viewport = ImGui::GetMainViewport();
        const ImVec2 topRight {viewport->Pos.x + viewport->Size.x - 16.f,
                               viewport->Pos.y + 16.f};
        ImGui::SetNextWindowPos(topRight, ImGuiCond_Always, {1.f, 0.f});

        const ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration
                                     | ImGuiWindowFlags_AlwaysAutoResize
                                     | ImGuiWindowFlags_NoInputs;
        ImGui::Begin("Dodge", nullptr, flags);
        ImGui::Text("Time %5.1f s    Best %5.1f s", Game.Survived, Game.Best);
        if (Game.bGameOver) {
            ImGui::TextColored({1.f, 0.4f, 0.4f, 1.f}, "Hit! Press R to restart.");
        } else {
            ImGui::TextDisabled("A / D or arrow keys to move");
        }
        ImGui::End();
    }
};
//! [hud]

int main() {
    //! [setup]
    // 1. Textures must exist before the engine starts:
    //    the renderer loads "CollisionTexture" during startup.
    RegisterSolidTexture("CollisionTexture", 8);
    RegisterSolidTexture("White", 8);

    Engine::InitializePlugins();
    flecs::world& world = ECS::GetWorld();

    // 2. Register our components (adds reflection data and an Entity Debugger panel).
    ECS_REGISTER_COMPONENT(FPlayer, world);
    ECS_REGISTER_COMPONENT(FFalling, world);
    ECS_REGISTER_COMPONENT(FSpawner, world);

    // 3. Bind keys. Hold bindings are active every frame the key is down;
    //    their values are added up.
    Input::AddInputBindings_Hold(&Input::Actions::MoveX, {
        {SDL_SCANCODE_A, -1.f}, {SDL_SCANCODE_LEFT, -1.f},
        {SDL_SCANCODE_D,  1.f}, {SDL_SCANCODE_RIGHT, 1.f},
    });
    Input::AddInputBindings_Press(&Input::Actions::Restart, { {SDL_SCANCODE_R, true} });

    // 4. Build the scene.
    CreateCameraAndBackground(world, {0.07f, 0.08f, 0.12f, 1.f});
    // Floor strip
    SpawnRect(world, {0.f, ScreenH - 6.f}, {ScreenW, 6.f}, {0.25f, 0.28f, 0.36f, 1.f});

    // Player, centred on the floor
    const glm::vec2 playerStart {(ScreenW - PlayerSize) / 2.f,
                                 ScreenH - 6.f - PlayerSize};
    SpawnRect(world, playerStart, {PlayerSize, PlayerSize}, {0.3f, 0.85f, 1.f, 1.f})
        .set_name("Player")
        .add<FPlayer>();
    world.entity("Spawner").set<FSpawner>({});
    //! [setup]

    //! [query]
    // A cached query over every falling block, used by the collision check below.
    auto fallingBlocks = world.query<const FTransformComponent,
                                     const FTexture2DComponent,
                                     const FFalling>();
    //! [query]

    // 5. Systems. They run every frame in Engine::Run(),
    //    in the order they are declared within a phase.

    //! [player_system]
    // Move the player with the MoveX action and check for hits.
    world.system<FTransformComponent,
                 const FTexture2DComponent,
                 const FPlayer>("Player")
        .each([fallingBlocks](flecs::iter& it, size_t, FTransformComponent& transform,
                              const FTexture2DComponent& sprite, const FPlayer&) {
            if (Game.bGameOver) {
                return;
            }

            const float speed = 150.f;
            const float moveX = Input::GetValue(Input::Actions::MoveX);
            transform.Location.x += moveX * speed * it.delta_time();
            const float maxX = ScreenW - sprite.TextureSize.x;
            transform.Location.x = std::clamp(transform.Location.x, 0.f, maxX);

            const glm::vec2 playerPos {transform.Location.x, transform.Location.y};
            const glm::vec2 playerSize = sprite.TextureSize;
            fallingBlocks.each([&](const FTransformComponent& blockTransform,
                                   const FTexture2DComponent& blockSprite,
                                   const FFalling&) {
                const glm::vec2 blockPos {blockTransform.Location.x,
                                          blockTransform.Location.y};
                const glm::vec2 blockSize = blockSprite.TextureSize;
                const bool hit = Overlaps(playerPos, playerSize, blockPos, blockSize);
                if (hit && !Game.bGameOver) {
                    Game.bGameOver = true;
                    Game.Best = std::max(Game.Best, Game.Survived);
                    Log::Dodge::Info("Hit after {:.1f} seconds", Game.Survived);
                }
            });
        });
    //! [player_system]

    //! [fall_system]
    // Move falling blocks down and destroy them once they leave the screen.
    world.system<FTransformComponent, const FFalling>("Fall")
        .each([](flecs::entity block, FTransformComponent& transform,
                 const FFalling& falling) {
            if (Game.bGameOver) {
                return;
            }
            transform.Location.y += falling.Speed * block.world().delta_time();
            if (transform.Location.y > ScreenH) {
                // Deferred until the end of the system, so it is safe while iterating.
                block.destruct();
            }
        });
    //! [fall_system]

    //! [spawn_system]
    // Spawn blocks on a timer that speeds up the longer you survive.
    // Also handles restarting.
    world.system<FSpawner>("Spawn")
        .each([fallingBlocks](flecs::iter& it, size_t, FSpawner& spawner) {
            if (Game.bGameOver) {
                if (Input::IsActionTriggered(Input::Actions::Restart)) {
                    fallingBlocks.each([](flecs::entity block,
                                          const FTransformComponent&,
                                          const FTexture2DComponent&,
                                          const FFalling&) {
                        block.destruct();
                    });
                    Game.Survived = 0.f;
                    Game.bGameOver = false;
                }
                return;
            }

            Game.Survived += it.delta_time();
            spawner.Timer -= it.delta_time();
            if (spawner.Timer > 0.f) {
                return;
            }
            spawner.Timer = std::max(0.12f, 0.6f - Game.Survived * 0.012f);

            const float size = RandomRange(8.f, 22.f);
            const glm::vec2 position {RandomRange(0.f, ScreenW - size), -size};
            const SDL_FColor color {1.f, RandomRange(0.25f, 0.55f), 0.2f, 1.f};
            const float fallSpeed = RandomRange(50.f, 90.f) + Game.Survived * 2.f;
            SpawnRect(it.world(), position, {size, size}, color)
                .set<FFalling>({ .Speed = fallSpeed });
        });
    //! [spawn_system]

    //! [run]
    // 6. UI, then run.
    CoreEditor::Open<DodgeHud>();
    Log::Dodge::Info("Dodge started. Close the window to quit.");

    Engine::Run();
    Engine::TeardownPlugins();

    // Known issue: returning from main() after TeardownPlugins() crashes in static
    // destructors.
    // Exit without running them (the engine has already flushed its logs).
    std::quick_exit(0);
    //! [run]
}
