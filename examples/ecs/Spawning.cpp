// ECS example: spawning and destroying entities.
//
// Controls: Space for a burst of 100 sparks. Hold E to spray sparks continuously.
//
// Shows:
//   - a spawner component that creates entities on a timer
//   - a flecs prefab that holds the components every spark shares
//   - a lifetime component, and a system that fades entities out and destroys them
//   - counting live entities with a query, shown in a HUD
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
#include <cmath>
#include <cstdlib>
#include <random>

// The engine's components (FTransformComponent, FTexture2DComponent, ...).
using namespace Components;

DECLARE_LOGGER(Spawning, debug)

DEFINE_INPUT_ACTION(Burst, bool)
DEFINE_INPUT_ACTION(Spray, bool)

namespace Components {
    //! [components]
    // Creates Rate entities per second while Active is above zero.
    DEFINE_COMPONENT(FSpawner,
        ECS_MEMBER(float, Rate, 60.f),
        ECS_MEMBER(float, Active, 0.f),
        ECS_MEMBER(float, Accumulator, 0.f))

    // Seconds left to live, and the starting value (used to fade out).
    DEFINE_COMPONENT(FLifetime,
        ECS_MEMBER(float, Remaining, 1.f),
        ECS_MEMBER(float, Total, 1.f))

    DEFINE_COMPONENT(FVelocity,
        ECS_MEMBER(glm::vec2, Value, glm::vec2(0, 0)))
    //! [components]
}

constexpr float ScreenW = 320.f, ScreenH = 180.f;
static std::mt19937 Rng { std::random_device{}() };
static int LiveSparks = 0;

static float RandomRange(float Min, float Max) {
    return std::uniform_real_distribution<float>(Min, Max)(Rng);
}

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

class SparkHud : public ImGUIWrapper::IEditorTool {
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
        ImGui::Begin("Sparks", nullptr, flags);
        ImGui::Text("Live sparks: %d", LiveSparks);
        ImGui::TextDisabled("Space: burst    Hold E: spray");
        ImGui::End();
    }
};

int main() {
    RegisterSolidTexture("CollisionTexture", 8);
    RegisterSolidTexture("White", 8);

    Engine::InitializePlugins();
    flecs::world& world = ECS::GetWorld();

    ECS_REGISTER_COMPONENT(FSpawner, world);
    ECS_REGISTER_COMPONENT(FLifetime, world);
    ECS_REGISTER_COMPONENT(FVelocity, world);

    Input::AddInputBindings_Press(&Input::Actions::Burst, {{SDL_SCANCODE_SPACE, true}});
    Input::AddInputBindings_Hold(&Input::Actions::Spray, {{SDL_SCANCODE_E, true}});

    CreateCameraAndBackground(world, {0.06f, 0.05f, 0.09f, 1.f});

    //! [prefab]
    // A prefab holds the components every spark starts with. Instances created with
    // is_a(prefab) get their own copy of each one, so they can be changed per spark.
    flecs::entity sparkPrefab = world.prefab("Spark")
        .set<FTexture2DComponent>({
            .Sprite = { .SpriteSheet = {"White"} },
            .TextureSize = {3.f, 3.f},
            .Color = {1.f, 0.8f, 0.3f, 1.f},
        })
        .set<FRenderingComponent>({ .RenderLayer = Renderer::Entities })
        .set<FLifetime>({});
    //! [prefab]

    //! [spawn]
    // Creates one spark at Origin, flying in a random direction.
    auto spawnSpark = [sparkPrefab](const flecs::world& World, glm::vec2 Origin) {
        const float angle = RandomRange(0.f, 6.2831853f);
        const float speed = RandomRange(20.f, 90.f);
        const float life = RandomRange(0.6f, 1.8f);
        const glm::vec2 velocity {std::cos(angle) * speed, std::sin(angle) * speed};
        World.entity()
            .is_a(sparkPrefab)
            .set<FTransformComponent>({ .Location = {Origin.x, Origin.y, 0.f} })
            .set<FVelocity>({ .Value = velocity })
            .set<FLifetime>({ .Remaining = life, .Total = life });
    };
    //! [spawn]

    world.entity("Fountain").set<FSpawner>({ .Rate = 120.f });

    //! [spawner_system]
    // The spawner system: turns elapsed time into a whole number of new sparks.
    world.system<FSpawner>("Spawn")
        .each([spawnSpark](flecs::iter& it, size_t, FSpawner& spawner) {
            const glm::vec2 origin {ScreenW / 2.f, ScreenH / 2.f};
            if (Input::IsActionTriggered(Input::Actions::Burst)) {
                for (int i = 0; i < 100; ++i) {
                    spawnSpark(it.world(), origin);
                }
            }

            const bool bSpraying = Input::IsActionTriggered(Input::Actions::Spray);
            spawner.Active = bSpraying ? 1.f : 0.f;
            if (spawner.Active <= 0.f) {
                spawner.Accumulator = 0.f;
                return;
            }
            spawner.Accumulator += spawner.Rate * it.delta_time();
            while (spawner.Accumulator >= 1.f) {
                spawnSpark(it.world(), origin);
                spawner.Accumulator -= 1.f;
            }
        });
    //! [spawner_system]

    world.system<FTransformComponent, FVelocity>("Move")
        .each([](flecs::iter& it, size_t, FTransformComponent& transform,
                 FVelocity& velocity) {
            velocity.Value.y += 40.f * it.delta_time();   // a little gravity
            transform.Location.x += velocity.Value.x * it.delta_time();
            transform.Location.y += velocity.Value.y * it.delta_time();
        });

    //! [lifetime_system]
    // Count down, fade out, and destroy when the time is up.
    world.system<FLifetime, FTexture2DComponent>("Lifetime")
        .each([](flecs::entity entity, FLifetime& lifetime,
                 FTexture2DComponent& sprite) {
            lifetime.Remaining -= entity.world().delta_time();
            if (lifetime.Remaining <= 0.f) {
                entity.destruct();   // deferred: applied after the system finishes
                return;
            }
            sprite.Color.a = lifetime.Remaining / lifetime.Total;
        });
    //! [lifetime_system]

    //! [count]
    // Count live sparks for the HUD. A query matches instances, not the prefab itself.
    flecs::query<const FLifetime> sparks = world.query<const FLifetime>();
    world.system("CountSparks")
        .kind(flecs::PostUpdate)
        .run([sparks](flecs::iter& it) {
            LiveSparks = sparks.count();
            while (it.next()) {}
        });
    //! [count]

    CoreEditor::Open<SparkHud>();
    Log::Spawning::Info("Press Space or hold E. Close the window to quit.");

    Engine::Run();
    Engine::TeardownPlugins();
    // Known issue: avoid the crash in static destructors after teardown.
    std::quick_exit(0);
}
