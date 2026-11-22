// ECS example: moving entities.
//
// Controls: W A S D or arrow keys to move.
//
// Shows:
//   - a 2D input action whose bindings add up (diagonals work)
//   - an FVelocity component and one system that moves everything that has one
//   - a system that only runs for the player (FPlayer component)
//   - wanderers that bounce off the edges of the world
//   - a camera that follows the player inside a world larger than the screen
//   - a light that moves with the player
// Uses no files.

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
#include <glm/geometric.hpp>
#include <cstdlib>
#include <random>

// The engine's components (FTransformComponent, FTexture2DComponent, ...).
using namespace Components;

DECLARE_LOGGER(Movement, debug)

//! [action]
// A 2D action. Every binding that is held adds its value, so W + D gives (1, -1).
DEFINE_INPUT_ACTION(Move, glm::vec2)
//! [action]

//! [components]
namespace Components {
    // World units per second.
    DEFINE_COMPONENT(FVelocity,
        ECS_MEMBER(glm::vec2, Value, glm::vec2(0, 0)))

    // Marks the entity the keyboard controls.
    DEFINE_COMPONENT(FPlayer,
        ECS_MEMBER(float, Speed, 90.f))

    // Marks entities that bounce off the edges of the world.
    DEFINE_COMPONENT(FBounceInWorld)
}
//! [components]

constexpr float ViewW = 320.f, ViewH = 180.f;     // what the camera shows
constexpr float WorldW = 640.f, WorldH = 360.f;   // the whole play area
constexpr float ActorSize = 12.f;

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

static flecs::entity SpawnRect(const flecs::world& World, glm::vec2 Position,
                               glm::vec2 Size, SDL_FColor Color,
                               Renderer::EInstanceLayer Layer = Renderer::Entities) {
    return World.entity()
        .set<FTransformComponent>({ .Location = {Position.x, Position.y, 0.f} })
        .set<FTexture2DComponent>({
            .Sprite = { .SpriteSheet = {"White"} },
            .TextureSize = Size,
            .Color = Color,
        })
        .set<FRenderingComponent>({ .RenderLayer = Layer });
}

class MovementHud : public ImGUIWrapper::IEditorTool {
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
        ImGui::Begin("Movement", nullptr, flags);
        ImGui::TextDisabled("WASD or arrow keys to move");
        ImGui::End();
    }
};

int main() {
    RegisterSolidTexture("CollisionTexture", 8);
    RegisterSolidTexture("White", 8);

    Engine::InitializePlugins();
    flecs::world& world = ECS::GetWorld();

    ECS_REGISTER_COMPONENT(FVelocity, world);
    ECS_REGISTER_COMPONENT(FPlayer, world);
    ECS_REGISTER_COMPONENT(FBounceInWorld, world);

    //! [bindings]
    Input::AddInputBindings_Hold(&Input::Actions::Move, {
        {SDL_SCANCODE_W, {0, -1}}, {SDL_SCANCODE_UP,    {0, -1}},
        {SDL_SCANCODE_S, {0,  1}}, {SDL_SCANCODE_DOWN,  {0,  1}},
        {SDL_SCANCODE_A, {-1, 0}}, {SDL_SCANCODE_LEFT,  {-1, 0}},
        {SDL_SCANCODE_D, {1,  0}}, {SDL_SCANCODE_RIGHT, {1,  0}},
    });
    //! [bindings]

    // World: a background covering the whole world,
    // plus a grid of dots so camera movement is visible.
    const SDL_FColor groundColor {0.12f, 0.14f, 0.2f, 1.f};
    const SDL_FColor dotColor {0.35f, 0.4f, 0.55f, 1.f};
    SpawnRect(world, {0, 0}, {WorldW, WorldH}, groundColor, Renderer::Background)
        .set_name("Background");
    for (float x = 20.f; x < WorldW; x += 40.f) {
        for (float y = 20.f; y < WorldH; y += 40.f) {
            SpawnRect(world, {x, y}, {2.f, 2.f}, dotColor, Renderer::Background);
        }
    }

    //! [player]
    // The player carries the camera and a light, so both follow it.
    // Camera Boundaries are (minX, maxX, minY, maxY) for the camera's top-left corner.
    const glm::vec2 worldCentre {WorldW / 2.f, WorldH / 2.f};
    SpawnRect(world, worldCentre, {ActorSize, ActorSize}, {0.4f, 0.9f, 1.f, 1.f})
        .set_name("Player")
        .set<FPlayer>({})
        .set<FVelocity>({})
        .set<FCameraComponent>({
            .CameraSize = {ViewW, ViewH},
            .Boundaries = {0.f, WorldW - ViewW, 0.f, WorldH - ViewH},
        })
        .set<FLightComponent>({
            .Radius = 0.6f,
            .Intensity = 4.f,
            .PositionOffset = {ActorSize / 2, ActorSize / 2},
        });
    //! [player]

    // A dim light over everything, so areas away from the player are not pitch black.
    world.entity("AmbientLight")
        .set<FTransformComponent>({})
        .set<FLightComponent>({ .Radius = 100.f, .Intensity = 0.8f });

    // Wanderers: random velocity, bounce off the world edges.
    std::mt19937 rng {42};
    std::uniform_real_distribution<float> posX(0.f, WorldW - ActorSize);
    std::uniform_real_distribution<float> posY(0.f, WorldH - ActorSize);
    std::uniform_real_distribution<float> dir(-1.f, 1.f);
    std::uniform_real_distribution<float> hue(0.4f, 1.f);
    const float wandererSize = ActorSize * 0.75f;
    for (int i = 0; i < 40; ++i) {
        const glm::vec2 heading = glm::vec2(dir(rng), dir(rng)) + 0.001f;
        const glm::vec2 velocity = glm::normalize(heading) * 40.f;
        const SDL_FColor color {hue(rng), 0.45f, 0.35f, 1.f};
        SpawnRect(world, {posX(rng), posY(rng)}, {wandererSize, wandererSize}, color)
            .set<FVelocity>({ .Value = velocity })
            .add<FBounceInWorld>();
    }

    //! [systems]
    // 1. Player input -> velocity. Only matches the entity with FPlayer.
    world.system<const FPlayer, FVelocity>("PlayerInput")
        .each([](const FPlayer& player, FVelocity& velocity) {
            glm::vec2 direction = Input::GetValue(Input::Actions::Move);
            if (glm::length(direction) > 1.f) {
                direction = glm::normalize(direction);   // no faster diagonals
            }
            velocity.Value = direction * player.Speed;
        });

    // 2. Velocity -> position, for every entity that has both components.
    world.system<FTransformComponent, const FVelocity>("ApplyVelocity")
        .each([](flecs::iter& it, size_t, FTransformComponent& transform,
                 const FVelocity& velocity) {
            transform.Location.x += velocity.Value.x * it.delta_time();
            transform.Location.y += velocity.Value.y * it.delta_time();
        });

    // 3. Keep everything inside the world.
    //    Bouncers reverse direction; the player just stops.
    world.system<FTransformComponent,
                 FVelocity,
                 const FTexture2DComponent>("StayInWorld")
        .kind(flecs::PostUpdate)
        .each([](flecs::entity entity, FTransformComponent& transform,
                 FVelocity& velocity, const FTexture2DComponent& sprite) {
            const bool bBounce = entity.has<FBounceInWorld>();
            const glm::vec2 max = glm::vec2(WorldW, WorldH) - sprite.TextureSize;
            for (int axis = 0; axis < 2; ++axis) {
                float& location = transform.Location[axis];
                if (location < 0.f || location > max[axis]) {
                    location = glm::clamp(location, 0.f, max[axis]);
                    if (bBounce) {
                        velocity.Value[axis] = -velocity.Value[axis];
                    }
                }
            }
        });
    //! [systems]

    CoreEditor::Open<MovementHud>();
    Log::Movement::Info("Move with WASD or the arrow keys. Close the window to quit.");

    Engine::Run();
    Engine::TeardownPlugins();
    // Known issue: avoid the crash in static destructors after teardown.
    std::quick_exit(0);
}
