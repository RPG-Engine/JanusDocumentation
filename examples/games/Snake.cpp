// Snake: steer the snake to eat the red food. Each bite makes it longer and faster.
// Hitting a wall or your own tail ends the round.
//
// Controls: W A S D or arrow keys to turn. R to restart after a crash.
//
// Shows: press bindings with the OVERRIDE operator, game logic on a fixed time step,
// keeping game data (grid cells) separate from presentation (a system that copies
// grid cells into transforms), reusing entities instead of destroying them, and
// changing a sprite's colour at runtime. Uses no files.

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
#include <deque>
#include <random>

// The engine's components (FTransformComponent, FTexture2DComponent, ...).
using namespace Components;

DECLARE_LOGGER(Snake, debug)

//! [actions]
// ---- Input actions ----------------------------------------------------------
// OVERRIDE: the action's value is the value of the last binding that triggered,
// instead of the sum of all of them. Exactly what a "turn" needs.
DEFINE_INPUT_ACTION_WITH_PARAMS(Turn, glm::vec2, OVERRIDE)
DEFINE_INPUT_ACTION(Restart, bool)
//! [actions]

//! [components]
// ---- Components -------------------------------------------------------------
namespace Components {
    // Position on the game grid, in cells. A system turns it into a transform.
    DEFINE_COMPONENT(FGridCell,
        ECS_MEMBER(glm::vec2, Cell, glm::vec2(0, 0)))

    DEFINE_COMPONENT(FSnake,
        ECS_MEMBER(glm::vec2, Direction, glm::vec2(1, 0)),
        ECS_MEMBER(glm::vec2, NextDirection, glm::vec2(1, 0)),
        ECS_MEMBER(float, StepTimer, 0.f))

    DEFINE_COMPONENT(FFood)
}
//! [components]

//! [state]
// ---- Game state -------------------------------------------------------------
constexpr float CellSize = 10.f;
constexpr int GridW = 32;            // 32 x 10 = 320 world units
constexpr int GridH = 18;            // 18 x 10 = 180 world units
constexpr SDL_FColor HeadColor {0.55f, 1.f, 0.45f, 1.f};
constexpr SDL_FColor BodyColor {0.2f, 0.7f, 0.3f, 1.f};

struct SSnakeState {
    std::deque<flecs::entity> Body;  // Body.front() is the head
    int Score = 0;
    int Best = 0;
    bool bGameOver = false;
};
static SSnakeState Game;
static std::mt19937 Rng { std::random_device{}() };
//! [state]

// ---- Scene helpers (the same in every example) -------------------------------
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

static void CreateCameraAndBackground(const flecs::world& World, SDL_FColor Color) {
    const glm::vec2 viewSize {GridW * CellSize, GridH * CellSize};

    World.entity("Camera")
        .set<FTransformComponent>({})
        .set<FCameraComponent>({ .CameraSize = viewSize, .Boundaries = {0, 0, 0, 0} })
        .set<FLightComponent>({ .Radius = 100.f, .Intensity = 2.4f });

    World.entity("Background")
        .set<FTransformComponent>({})
        .set<FTexture2DComponent>({
            .Sprite = { .SpriteSheet = {"White"} },
            .TextureSize = viewSize,
            .Color = Color,
        })
        .set<FRenderingComponent>({ .RenderLayer = Renderer::Background });
}

//! [spawn_cell]
// A grid-aligned square. The transform is filled in by the SnapToGrid system.
static flecs::entity SpawnCell(const flecs::world& World, glm::vec2 Cell,
                               SDL_FColor Color) {
    return World.entity()
        .set<FGridCell>({ .Cell = Cell })
        .set<FTransformComponent>({})
        .set<FTexture2DComponent>({
            .Sprite = { .SpriteSheet = {"White"} },
            .TextureSize = {CellSize - 1.f, CellSize - 1.f},   // 1 px gap between cells
            .Color = Color,
        })
        .set<FRenderingComponent>({ .RenderLayer = Renderer::Entities });
}
//! [spawn_cell]

static void SetColor(flecs::entity Entity, SDL_FColor Color) {
    Entity.get_mut<FTexture2DComponent>().Color = Color;
}

static bool IsOnSnake(glm::vec2 Cell) {
    return std::ranges::any_of(Game.Body, [Cell](flecs::entity segment) {
        // try_get: a segment created this frame inside a system gets its
        // components at the end of the frame's deferred block, so it may not
        // have an FGridCell yet.
        const FGridCell* grid = segment.try_get<FGridCell>();
        return grid != nullptr && grid->Cell == Cell;
    });
}

static glm::vec2 RandomFreeCell(glm::vec2 AlsoAvoid) {
    std::uniform_int_distribution<int> x(0, GridW - 1), y(0, GridH - 1);
    glm::vec2 cell;
    do {
        cell = {static_cast<float>(x(Rng)), static_cast<float>(y(Rng))};
    } while (cell == AlsoAvoid || IsOnSnake(cell));
    return cell;
}

//! [start_round]
static void StartRound(const flecs::world& World) {
    for (flecs::entity segment : Game.Body) {
        segment.destruct();
    }
    Game.Body.clear();
    for (int i = 0; i < 4; ++i) {
        const SDL_FColor color = (i == 0) ? HeadColor : BodyColor;
        Game.Body.push_back(SpawnCell(World, {8.f - i, 9.f}, color));
    }

    World.lookup("Snake").set<FSnake>({});
    World.lookup("Food").set<FGridCell>({ .Cell = {20.f, 9.f} });
    Game.Score = 0;
    Game.bGameOver = false;
}
//! [start_round]

// ---- HUD ----------------------------------------------------------------------
class SnakeHud : public ImGUIWrapper::IEditorTool {
protected:
    void Draw_Internal() override {
        ImGui::SetWindowCollapsed("Dear ImGui Demo", true, ImGuiCond_Once);

        const ImGuiViewport* viewport = ImGui::GetMainViewport();
        const ImVec2 topRight {viewport->Pos.x + viewport->Size.x - 16.f,
                               viewport->Pos.y + 16.f};
        ImGui::SetNextWindowPos(topRight, ImGuiCond_Always, {1.f, 0.f});
        const ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration
                                     | ImGuiWindowFlags_AlwaysAutoResize
                                     | ImGuiWindowFlags_NoInputs;
        ImGui::Begin("Snake", nullptr, flags);
        ImGui::Text("Score %d    Best %d", Game.Score, Game.Best);
        if (Game.bGameOver) {
            ImGui::TextColored({1.f, 0.4f, 0.4f, 1.f}, "Crashed! Press R to restart.");
        } else {
            ImGui::TextDisabled("WASD or arrow keys to turn");
        }
        ImGui::End();
    }
};

int main() {
    RegisterSolidTexture("CollisionTexture", 8);
    RegisterSolidTexture("White", 8);

    Engine::InitializePlugins();
    flecs::world& world = ECS::GetWorld();

    ECS_REGISTER_COMPONENT(FGridCell, world);
    ECS_REGISTER_COMPONENT(FSnake, world);
    ECS_REGISTER_COMPONENT(FFood, world);

    //! [bindings]
    // Press bindings trigger once, on the frame the key goes down.
    Input::AddInputBindings_Press(&Input::Actions::Turn, {
        {SDL_SCANCODE_W, {0, -1}}, {SDL_SCANCODE_UP, {0, -1}},
        {SDL_SCANCODE_S, {0,  1}}, {SDL_SCANCODE_DOWN, {0,  1}},
        {SDL_SCANCODE_A, {-1, 0}}, {SDL_SCANCODE_LEFT, {-1, 0}},
        {SDL_SCANCODE_D, {1,  0}}, {SDL_SCANCODE_RIGHT, {1,  0}},
    });
    Input::AddInputBindings_Press(&Input::Actions::Restart, { {SDL_SCANCODE_R, true} });
    //! [bindings]

    CreateCameraAndBackground(world, {0.05f, 0.09f, 0.07f, 1.f});
    world.entity("Snake").set<FSnake>({});
    SpawnCell(world, {0, 0}, {1.f, 0.3f, 0.3f, 1.f}).set_name("Food").add<FFood>();
    StartRound(world);

    //! [step]
    // Game logic: read turns every frame, move one cell every step.
    world.system<FSnake>("SnakeStep")
        .each([](flecs::iter& it, size_t, FSnake& snake) {
            if (Game.bGameOver) {
                if (Input::IsActionTriggered(Input::Actions::Restart)) {
                    StartRound(it.world());
                }
                return;
            }

            // Turn, but never straight back into yourself.
            const glm::vec2 turn = Input::GetValue(Input::Actions::Turn);
            if (turn != glm::vec2(0, 0) && turn != -snake.Direction) {
                snake.NextDirection = turn;
            }

            snake.StepTimer -= it.delta_time();
            if (snake.StepTimer > 0.f) {
                return;
            }
            snake.StepTimer += std::max(0.05f, 0.12f - Game.Score * 0.003f);
            snake.Direction = snake.NextDirection;

            const glm::vec2 head = Game.Body.front().get<FGridCell>().Cell;
            const glm::vec2 newHead = head + snake.Direction;
            const bool bHitWall = newHead.x < 0 || newHead.x >= GridW
                               || newHead.y < 0 || newHead.y >= GridH;
            if (bHitWall || IsOnSnake(newHead)) {
                Game.bGameOver = true;
                Game.Best = std::max(Game.Best, Game.Score);
                Log::Snake::Info("Crashed with a score of {}", Game.Score);
                return;
            }

            SetColor(Game.Body.front(), BodyColor);
            flecs::entity food = it.world().lookup("Food");
            if (food.get<FGridCell>().Cell == newHead) {
                // Grow: move the food, then add a new head.
                food.set<FGridCell>({ .Cell = RandomFreeCell(newHead) });
                Game.Body.push_front(SpawnCell(it.world(), newHead, HeadColor));
                ++Game.Score;
            } else {
                // Move: reuse the tail segment as the new head,
                // instead of destroying one entity and creating another.
                flecs::entity tail = Game.Body.back();
                Game.Body.pop_back();
                tail.set<FGridCell>({ .Cell = newHead });
                SetColor(tail, HeadColor);
                Game.Body.push_front(tail);
            }
        });
    //! [step]

    //! [snap]
    // Presentation: place every grid entity in the world.
    // Runs after the game logic (PostUpdate).
    world.system<const FGridCell, FTransformComponent>("SnapToGrid")
        .kind(flecs::PostUpdate)
        .each([](const FGridCell& grid, FTransformComponent& transform) {
            transform.Location = {grid.Cell.x * CellSize, grid.Cell.y * CellSize, 0.f};
        });
    //! [snap]

    CoreEditor::Open<SnakeHud>();
    Log::Snake::Info("Snake started. Close the window to quit.");

    Engine::Run();
    Engine::TeardownPlugins();
    // Known issue: avoid the crash in static destructors after teardown.
    std::quick_exit(0);
}
