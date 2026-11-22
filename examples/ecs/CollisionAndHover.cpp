// ECS example: colliders, the debug-collider overlay, and mouse hover.
//
// Controls: W A S D to move. F1 toggles the collider overlay.
//           Hover a block with the mouse to highlight it;
//           click to select it in the Entity Debugger.
//
// Shows:
//   - GameplayFramework::Module(), which registers FColliderComponent and FMouseState
//   - the engine's Move2D action and ToggleDebugColliders action
//   - blocking movement against colliders, one axis at a time
//   - drawing collider outlines with the renderer's debug-collider container
//   - GameplayFramework::ActivateMouseStateSystem() and FBoundingBoxComponent for hover
// Uses no files.

#include <Module.hpp>
#include <Log/Module.hpp>
#include <Input/Module.hpp>
#include <Editor/Module.hpp>
#include <Filesystem/Module.hpp>
#include <Types/Module.hpp>
#include <Types/Graphics.hpp>
#include <Gameplay/Module.hpp>
#include <Gameplay/Components.hpp>
#include <Gameplay/ECS/Module.hpp>
#include <Gameplay/ECS/Modules/TransformModule.hpp>
#include <Gameplay/ECS/Modules/GraphicsModule.hpp>
#include <Rendering/Module.hpp>
#include <Rendering/Components/RenderingComponent.hpp>

#include <imgui.h>
#include <glm/geometric.hpp>
#include <cstdlib>

// The engine's components (FTransformComponent, FTexture2DComponent, ...).
using namespace Components;

DECLARE_LOGGER(Collision, debug)

namespace Components {
    DEFINE_COMPONENT(FPlayer)
}

constexpr float ScreenW = 320.f, ScreenH = 180.f;
constexpr SDL_FColor BlockColor {0.45f, 0.42f, 0.55f, 1.f};
constexpr SDL_FColor HoverColor {0.95f, 0.8f, 0.35f, 1.f};

// ---- Textures --------------------------------------------------------------------
//! [outline_texture]
// The debug-collider overlay draws every collider with the "CollisionTexture" asset.
// Generate an outline: an opaque 1-pixel border around a faint interior.
static void RegisterOutlineTexture(const std::string& AssetPath, int Size) {
    SDL_Surface* surface = SDL_CreateSurface(Size, Size, SDL_PIXELFORMAT_ABGR8888);
    const Uint32 border = SDL_MapSurfaceRGBA(surface, 255, 60, 60, 255);
    const Uint32 interior = SDL_MapSurfaceRGBA(surface, 255, 60, 60, 40);
    const SDL_Rect inner {1, 1, Size - 2, Size - 2};
    SDL_FillSurfaceRect(surface, nullptr, border);
    SDL_FillSurfaceRect(surface, &inner, interior);

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
//! [outline_texture]

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

//! [solid_block]
// A visible block with a collider and a bounding box (for mouse hover),
// both covering the whole sprite.
// This example stores ColliderShape as (x offset, y offset, width, height).
static flecs::entity SpawnSolidBlock(const flecs::world& World, glm::vec2 Position,
                                     glm::vec2 Size, SDL_FColor Color) {
    return World.entity()
        .set<FTransformComponent>({ .Location = {Position.x, Position.y, 0.f} })
        .set<FTexture2DComponent>({
            .Sprite = { .SpriteSheet = {"White"} },
            .TextureSize = Size,
            .Color = Color,
        })
        .set<FRenderingComponent>({ .RenderLayer = Renderer::Entities })
        .set<FColliderComponent>({ .ColliderShape = {0.f, 0.f, Size.x, Size.y} })
        .set<FBoundingBoxComponent>({ .Height = Size.y, .Width = Size.x });
}
//! [solid_block]

// The collider's rectangle in world coordinates.
static SDL_FRect ColliderRect(const FTransformComponent& Transform,
                              const FColliderComponent& Collider) {
    const glm::vec4& shape = Collider.ColliderShape;
    return {Transform.Location.x + shape.x, Transform.Location.y + shape.y,
            shape.z, shape.w};
}

class CollisionHud : public ImGUIWrapper::IEditorTool {
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
        ImGui::Begin("Collision", nullptr, flags);
        const bool bOverlay = CoreTypes::bShouldDisplayDebugColliders;
        ImGui::Text("Collider overlay: %s", bOverlay ? "on" : "off");
        ImGui::TextDisabled("WASD: move   F1: overlay   Click: select hovered block");
        ImGui::End();
    }
};

int main() {
    RegisterOutlineTexture("CollisionTexture", 16);
    RegisterSolidTexture("White", 8);

    Engine::InitializePlugins();
    flecs::world& world = ECS::GetWorld();

    //! [gameplay_module]
    // The engine does not call GameplayFramework::Module(). Call it yourself before
    // using FColliderComponent or FMouseState, so they get reflection data and
    // Entity Debugger panels.
    GameplayFramework::Module();
    ECS_REGISTER_COMPONENT(FPlayer, world);
    //! [gameplay_module]

    //! [bindings]
    // The engine declares Move2D in Gameplay/Module.hpp and ToggleDebugColliders
    // in Types/Module.hpp. It binds no keys to either; that is up to the game.
    Input::AddInputBindings_Hold(&Input::Actions::Move2D, {
        {SDL_SCANCODE_W, {0, -1}}, {SDL_SCANCODE_S, {0, 1}},
        {SDL_SCANCODE_A, {-1, 0}}, {SDL_SCANCODE_D, {1, 0}},
    });
    Input::AddInputBindings_Press(&Input::Actions::ToggleDebugColliders, {
        {SDL_SCANCODE_F1, true},
    });
    //! [bindings]

    CreateCameraAndBackground(world, {0.11f, 0.12f, 0.1f, 1.f});

    // Walls around the edge, and a few pillars.
    SpawnSolidBlock(world, {0, 0}, {ScreenW, 8}, BlockColor);
    SpawnSolidBlock(world, {0, ScreenH - 8}, {ScreenW, 8}, BlockColor);
    SpawnSolidBlock(world, {0, 8}, {8, ScreenH - 16}, BlockColor);
    SpawnSolidBlock(world, {ScreenW - 8, 8}, {8, ScreenH - 16}, BlockColor);
    for (int i = 0; i < 4; ++i) {
        const glm::vec2 position {50.f + i * 65.f, 40.f + (i % 2) * 50.f};
        const std::string name = "Pillar " + std::to_string(i);
        SpawnSolidBlock(world, position, {24.f, 40.f}, BlockColor)
            .set_name(name.c_str());
    }

    // The player: a small sprite with a collider only at its feet,
    // like a top-down character.
    world.entity("Player")
        .set<FTransformComponent>({ .Location = {20.f, 140.f, 0.f} })
        .set<FTexture2DComponent>({
            .Sprite = { .SpriteSheet = {"White"} },
            .TextureSize = {12.f, 16.f},
            .Color = {0.4f, 0.9f, 1.f, 1.f},
        })
        .set<FRenderingComponent>({ .RenderLayer = Renderer::Entities })
        .set<FColliderComponent>({ .ColliderShape = {0.f, 10.f, 12.f, 6.f} })
        .add<FPlayer>();

    auto colliders = world.query_builder<const FTransformComponent,
                                         const FColliderComponent>()
        .without<FPlayer>()
        .build();

    //! [blocking_movement]
    // Move one axis at a time, and undo the move on an axis that would overlap
    // a collider. That lets the player slide along walls.
    // F1 (ToggleDebugColliders) switches the overlay.
    world.system<FTransformComponent,
                 const FColliderComponent,
                 const FPlayer>("MovePlayer")
        .each([colliders](flecs::iter& it, size_t, FTransformComponent& transform,
                          const FColliderComponent& collider, const FPlayer&) {
            if (Input::IsActionTriggered(Input::Actions::ToggleDebugColliders)) {
                bool& bOverlay = CoreTypes::bShouldDisplayDebugColliders;
                bOverlay = !bOverlay;
            }

            glm::vec2 move = Input::GetValue(Input::Actions::Move2D);
            if (glm::length(move) > 1.f) {
                move = glm::normalize(move);
            }
            move *= 80.f * it.delta_time();

            for (int axis = 0; axis < 2; ++axis) {
                transform.Location[axis] += move[axis];
                const SDL_FRect playerRect = ColliderRect(transform, collider);
                bool bBlocked = false;
                colliders.each([&](const FTransformComponent& otherTransform,
                                   const FColliderComponent& otherCollider) {
                    const SDL_FRect other = ColliderRect(otherTransform, otherCollider);
                    if (SDL_HasRectIntersectionFloat(&playerRect, &other)) {
                        bBlocked = true;
                    }
                });
                if (bBlocked) {
                    transform.Location[axis] -= move[axis];
                }
            }
        });
    //! [blocking_movement]

    //! [debug_overlay]
    // Fill the renderer's debug-collider container. It is drawn on top of everything
    // while CoreTypes::bShouldDisplayDebugColliders is true (toggled here by the
    // ToggleDebugColliders action).
    world.system<const FTransformComponent, const FColliderComponent>("DebugColliders")
        .kind(ECS::Phase::OnPreRender())
        .each([](flecs::entity entity, const FTransformComponent& transform,
                 const FColliderComponent& collider) {
            const SDL_FRect rect = ColliderRect(transform, collider);
            const uint32_t id = static_cast<uint32_t>(entity.id());
            auto& container = Renderer::CollisionInstances;
            container.AddEntity(id);
            Renderer::SpriteInstance& instance = container.Instances[id];
            instance.x = rect.x;
            instance.y = rect.y;
            instance.w = rect.w;
            instance.h = rect.h;
            instance.tex_w = 1.f;
            instance.tex_h = 1.f;
            instance.r = instance.g = instance.b = instance.a = 1.f;
        });
    //! [debug_overlay]

    //! [hover]
    // The engine's mouse-hover system writes the entity under the cursor into the
    // FMouseState component on ECS::GetGlobalSystemsEntity(). It checks entities
    // with FTransformComponent and FBoundingBoxComponent.
    GameplayFramework::ActivateMouseStateSystem();

    world.system<const FTransformComponent,
                 const FBoundingBoxComponent,
                 FTexture2DComponent>("Highlight")
        .each([](flecs::entity entity, const FTransformComponent& transform,
                 const FBoundingBoxComponent& box, FTexture2DComponent& sprite) {
            const FMouseState& mouse = ECS::GetGlobalSystemsEntity().get<FMouseState>();

            // HoveredEntityId keeps the last entity hovered,
            // so also check that the cursor is still inside the box.
            const glm::vec2 cursor = mouse.MousePosition;
            const glm::vec2 min {transform.Location.x, transform.Location.y};
            const glm::vec2 max = min + glm::vec2(box.Width, box.Height);
            const bool bInside = cursor.x >= min.x && cursor.x <= max.x
                              && cursor.y >= min.y && cursor.y <= max.y;
            const bool bHovered = mouse.HoveredEntityId == entity.id() && bInside;
            sprite.Color = bHovered ? HoverColor : BlockColor;

            const SDL_MouseButtonFlags buttons = SDL_GetMouseState(nullptr, nullptr);
            const bool bClicked = buttons & SDL_BUTTON_LMASK;
            if (bHovered && bClicked) {
                // Shows it in Entity Debugger > Entity Outliner
                ECS::SelectEntity(entity);
            }
        });
    //! [hover]

    CoreEditor::Open<CollisionHud>();
    Log::Collision::Info("WASD to move, F1 for the collider overlay.");
    Log::Collision::Info("Close the window to quit.");

    Engine::Run();
    Engine::TeardownPlugins();
    // Known issue: avoid the crash in static destructors after teardown.
    std::quick_exit(0);
}
