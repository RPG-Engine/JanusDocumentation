// ECS example: components, tags, queries, observers and phases.
//
// Thirty units wander around, each on the red or the blue team.
// Controls: 1 moves a red unit to the blue team, 2 does the opposite,
//           H damages every blue unit (they are destroyed at 0 health).
//
// Shows:
//   - the three DEFINE_COMPONENT variants (default, NO_DEBUG, OVERRIDE_DEBUG)
//   - plain flecs tags, and queries that filter on them (with / without)
//   - observers that react when a tag is added or an entity is destroyed
//   - a custom pipeline phase made with DEFINE_NEW_ECS_PHASE
//   - settings stored on the engine's global entity (ECS::GetGlobalSystemsEntity)
//   - selecting an entity for the Entity Debugger with ECS::SelectEntity
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

DECLARE_LOGGER(Teams, debug)

DEFINE_INPUT_ACTION(ToBlue, bool)
DEFINE_INPUT_ACTION(ToRed, bool)
DEFINE_INPUT_ACTION(HurtBlue, bool)

//! [phase]
// A new pipeline phase that runs after flecs::PostUpdate.
// Use it with .kind(ECS::Phase::OnLateGameplay()).
DEFINE_NEW_ECS_PHASE(OnLateGameplay, PostUpdate)
//! [phase]

//! [components]
namespace Components {
    // 1. DEFINE_COMPONENT: a struct with reflection data
    //    and an automatic Entity Debugger panel.
    DEFINE_COMPONENT(FHealth,
        ECS_MEMBER(float, Current, 100.f),
        ECS_MEMBER(float, Max, 100.f))

    // 2. DEFINE_COMPONENT_NO_DEBUG: reflection data, but no Entity Debugger panel.
    DEFINE_COMPONENT_NO_DEBUG(FWander,
        ECS_MEMBER(glm::vec2, Target, glm::vec2(0, 0)),
        ECS_MEMBER(float, Speed, 30.f))

    // 3. DEFINE_COMPONENT_OVERRIDE_DEBUG: you supply functions that draw before
    //    and after the automatic member widgets. Declare them first; the component
    //    type is not defined yet.
    struct FUnit;
    void PreDrawUnit(FUnit* Unit);
    void PostDrawUnit(FUnit* Unit);
    DEFINE_COMPONENT_OVERRIDE_DEBUG(FUnit, PreDrawUnit, PostDrawUnit,
        ECS_MEMBER(float, Size, 8.f))

    // Settings shared by every unit, stored on the engine's global entity.
    DEFINE_COMPONENT(FTeamSettings,
        ECS_MEMBER(float, DamagePerHit, 25.f),
        ECS_MEMBER(SDL_FColor, RedColor, SDL_FColor(1.f, 0.35f, 0.35f, 1.f)),
        ECS_MEMBER(SDL_FColor, BlueColor, SDL_FColor(0.35f, 0.6f, 1.f, 1.f)))
}

// Plain flecs tags: empty structs. They need no registration and carry no data.
struct RedTeam {};
struct BlueTeam {};
//! [components]

//! [debug_draw]
namespace Components {
    void PreDrawUnit(FUnit* Unit) {
        ImGui::TextDisabled("Custom panel drawn by PreDrawUnit");
    }
    void PostDrawUnit(FUnit* Unit) {
        if (ImGui::Button("Grow")) {
            Unit->Size += 2.f;
        }
    }
}
//! [debug_draw]

constexpr float ScreenW = 320.f, ScreenH = 180.f;
static std::mt19937 Rng {7};

static glm::vec2 RandomPoint() {
    return {std::uniform_real_distribution<float>(10.f, ScreenW - 20.f)(Rng),
            std::uniform_real_distribution<float>(10.f, ScreenH - 20.f)(Rng)};
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

//! [queries]
// Queries are created once and reused. These two filter on the team tags.
static flecs::query<const FUnit> RedUnits;
static flecs::query<const FUnit> BlueUnits;
static flecs::query<FHealth> BlueHealth;
//! [queries]

class TeamsHud : public ImGUIWrapper::IEditorTool {
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
        ImGui::Begin("Teams", nullptr, flags);
        const int red = RedUnits.count();
        const int blue = BlueUnits.count();
        ImGui::Text("Red units: %d    Blue units: %d", red, blue);
        ImGui::TextDisabled("1: red -> blue   2: blue -> red   H: hurt blue");
        ImGui::TextDisabled("Open Entity Debugger > Entity Outliner to inspect units");
        ImGui::End();
    }
};

// Moves the first entity a query finds from one team to the other.
template <typename TFrom, typename TTo>
static void SwitchOneUnit(const flecs::query<const FUnit>& From) {
    const flecs::entity unit = From.first();
    if (unit.is_alive()) {
        unit.remove<TFrom>().add<TTo>();
    }
}

int main() {
    RegisterSolidTexture("CollisionTexture", 8);
    RegisterSolidTexture("White", 8);

    Engine::InitializePlugins();
    flecs::world& world = ECS::GetWorld();

    //! [register]
    ECS_REGISTER_COMPONENT(FHealth, world);
    ECS_REGISTER_COMPONENT_NO_DEBUG(FWander, world);
    ECS_REGISTER_COMPONENT(FUnit, world);
    ECS_REGISTER_COMPONENT(FTeamSettings, world);
    //! [register]

    Input::AddInputBindings_Press(&Input::Actions::ToBlue, {{SDL_SCANCODE_1, true}});
    Input::AddInputBindings_Press(&Input::Actions::ToRed, {{SDL_SCANCODE_2, true}});
    Input::AddInputBindings_Press(&Input::Actions::HurtBlue, {{SDL_SCANCODE_H, true}});

    CreateCameraAndBackground(world, {0.1f, 0.1f, 0.12f, 1.f});

    //! [global_entity]
    // The global entity is listed under "Global Entity" in the Entity Debugger,
    // so these settings can be edited live.
    flecs::entity settingsEntity = ECS::GetGlobalSystemsEntity();
    settingsEntity.set<FTeamSettings>({});
    //! [global_entity]

    //! [build_queries]
    RedUnits = world.query_builder<const FUnit>().with<RedTeam>().build();
    BlueUnits = world.query_builder<const FUnit>().with<BlueTeam>().build();
    BlueHealth = world.query_builder<FHealth>().with<BlueTeam>().build();
    //! [build_queries]

    //! [observers]
    // Observers run immediately when something happens to an entity,
    // instead of every frame.
    world.observer<FTexture2DComponent>("PaintRed")
        .with<RedTeam>()
        .event(flecs::OnAdd)
        .each([](FTexture2DComponent& sprite) {
            sprite.Color = ECS::GetGlobalSystemsEntity().get<FTeamSettings>().RedColor;
        });
    world.observer<FTexture2DComponent>("PaintBlue")
        .with<BlueTeam>()
        .event(flecs::OnAdd)
        .each([](FTexture2DComponent& sprite) {
            sprite.Color = ECS::GetGlobalSystemsEntity().get<FTeamSettings>().BlueColor;
        });
    // OnRemove also fires when an entity is destroyed.
    // The component can still be read here.
    world.observer<const FHealth>("UnitDefeated")
        .event(flecs::OnRemove)
        .each([](flecs::entity unit, const FHealth& health) {
            if (health.Current <= 0.f) {
                Log::Teams::Info("{} was defeated", unit.name().c_str());
            }
        });
    //! [observers]

    // Spawn the units. The team tag is added last, so the observers see the sprite.
    for (int i = 0; i < 30; ++i) {
        const glm::vec2 position = RandomPoint();
        flecs::entity unit = world.entity(("Unit " + std::to_string(i)).c_str())
            .set<FTransformComponent>({ .Location = {position.x, position.y, 0.f} })
            .set<FTexture2DComponent>({
                .Sprite = { .SpriteSheet = {"White"} },
                .TextureSize = {8.f, 8.f},
            })
            .set<FRenderingComponent>({ .RenderLayer = Renderer::Entities })
            .set<FUnit>({})
            .set<FHealth>({})
            .set<FWander>({ .Target = RandomPoint() });
        if (i % 2 == 0) {
            unit.add<RedTeam>();
        } else {
            unit.add<BlueTeam>();
        }
    }
    ECS::SelectEntity(world.lookup("Unit 0"));   // shown in the Entity Debugger

    //! [systems]
    // Input: act on the queries.
    world.system<const FTeamSettings>("TeamInput")
        .each([](const FTeamSettings& settings) {
            if (Input::IsActionTriggered(Input::Actions::ToBlue)) {
                SwitchOneUnit<RedTeam, BlueTeam>(RedUnits);
            }
            if (Input::IsActionTriggered(Input::Actions::ToRed)) {
                SwitchOneUnit<BlueTeam, RedTeam>(BlueUnits);
            }
            if (Input::IsActionTriggered(Input::Actions::HurtBlue)) {
                BlueHealth.each([&settings](FHealth& health) {
                    health.Current -= settings.DamagePerHit;
                });
            }
        });

    // Wander towards a target; pick a new one on arrival.
    // FUnit::Size drives the sprite size.
    world.system<FTransformComponent,
                 FWander,
                 FTexture2DComponent,
                 const FUnit>("Wander")
        .each([](flecs::iter& it, size_t, FTransformComponent& transform,
                 FWander& wander, FTexture2DComponent& sprite, const FUnit& unit) {
            const glm::vec2 position {transform.Location.x, transform.Location.y};
            const glm::vec2 toTarget = wander.Target - position;
            if (glm::length(toTarget) < 2.f) {
                wander.Target = RandomPoint();
                return;
            }
            const float distance = wander.Speed * it.delta_time();
            const glm::vec2 step = glm::normalize(toTarget) * distance;
            transform.Location.x += step.x;
            transform.Location.y += step.y;
            sprite.TextureSize = {unit.Size, unit.Size};
        });

    // Runs in the custom phase, after all gameplay:
    // remove units that ran out of health.
    world.system<const FHealth>("RemoveDead")
        .kind(ECS::Phase::OnLateGameplay())
        .each([](flecs::entity entity, const FHealth& health) {
            if (health.Current <= 0.f) {
                entity.destruct();
            }
        });
    //! [systems]

    CoreEditor::Open<TeamsHud>();
    Log::Teams::Info("Press 1, 2 or H. Close the window to quit.");

    Engine::Run();
    Engine::TeardownPlugins();
    // Known issue: avoid the crash in static destructors after teardown.
    std::quick_exit(0);
}
