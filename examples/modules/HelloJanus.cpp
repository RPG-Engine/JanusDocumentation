// Hello Janus: the smallest program that puts something on screen.
//
// It shows the four things every Janus program needs before anything is visible:
//   1. a texture for the debug-collider overlay, registered as "CollisionTexture"
//   2. a camera entity
//   3. at least one light
//   4. a sprite on the Background render layer (it clears the screen each frame)
// It uses no files: textures are generated in memory.

#include <Module.hpp>
#include <Log/Module.hpp>
#include <Filesystem/Module.hpp>
#include <Types/Graphics.hpp>
#include <Gameplay/ECS/Module.hpp>
#include <Gameplay/ECS/Modules/TransformModule.hpp>
#include <Gameplay/ECS/Modules/GraphicsModule.hpp>
#include <Rendering/Components/RenderingComponent.hpp>

#include <cstdlib>

// The engine's components (FTransformComponent, FTexture2DComponent, ...).
using namespace Components;

DECLARE_LOGGER(Hello, debug)

//! [solid_texture]
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
    assetData->ObjectData = texture;   // already "loaded": LoadFile() returns it
    Filesystem::RegisterAssetData(assetData);
}
//! [solid_texture]

int main() {
    //! [main]
    // The renderer loads "CollisionTexture" while the engine starts,
    // so register it first.
    RegisterSolidTexture("CollisionTexture", 8);
    RegisterSolidTexture("White", 8);

    Engine::InitializePlugins();
    flecs::world& world = ECS::GetWorld();

    // Camera: shows world units (0,0) to (320,180). Boundaries of 0 pin it in place.
    // The light on the same entity is large enough to light the whole screen.
    world.entity("Camera")
        .set<FTransformComponent>({})
        .set<FCameraComponent>({ .CameraSize = {320, 180}, .Boundaries = {0, 0, 0, 0} })
        .set<FLightComponent>({ .Radius = 100.f, .Intensity = 2.4f });

    // Background: a full-screen sprite on the Background layer.
    world.entity("Background")
        .set<FTransformComponent>({})
        .set<FTexture2DComponent>({
            .Sprite = { .SpriteSheet = {"White"} },
            .TextureSize = {320, 180},
            .Color = {0.10f, 0.12f, 0.18f, 1.f} })
        .set<FRenderingComponent>({ .RenderLayer = Renderer::Background });

    // A 32x32 orange square. Location is its top-left corner, in world units.
    world.entity("Square")
        .set<FTransformComponent>({ .Location = {48, 100, 0} })
        .set<FTexture2DComponent>({
            .Sprite = { .SpriteSheet = {"White"} },
            .TextureSize = {32, 32},
            .Color = {1.f, 0.55f, 0.1f, 1.f} })
        .set<FRenderingComponent>({ .RenderLayer = Renderer::Entities });

    Log::Hello::Info("Hello, Janus! Close the window to quit.");

    Engine::Run();             // returns when the window is closed
    Engine::TeardownPlugins();

    // Known issue: returning from main() after TeardownPlugins() crashes in static
    // destructors.
    // Exit without running them (the engine has already flushed its logs).
    std::quick_exit(0);
    //! [main]
}
