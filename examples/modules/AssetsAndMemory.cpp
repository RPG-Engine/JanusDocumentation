// Module example: assets (Filesystem) and TObjectPtr (Memory).
//
// Prints what the Filesystem module found under Resources/, loads a text asset that
// ships with the engine (a shader source), shows TObjectPtr in use, and displays a
// checkerboard texture generated at runtime. A window lists every registered asset.
//
// Shows: GetAllAssetPaths, GetAssetData and SAssetData, LoadFile / AssetExists /
// IsValid, STextFileAsset, registering an asset made in code, TObjectPtr and CastTo.
// Uses no files of its own.

#include <Module.hpp>
#include <Log/Module.hpp>
#include <Editor/Module.hpp>
#include <Filesystem/Module.hpp>
#include <Filesystem/Importers/Importer.hpp>
#include <Memory/Module.hpp>
#include <Types/Graphics.hpp>
#include <Gameplay/ECS/Module.hpp>
#include <Gameplay/ECS/Modules/TransformModule.hpp>
#include <Gameplay/ECS/Modules/GraphicsModule.hpp>
#include <Rendering/Components/RenderingComponent.hpp>

#include <imgui.h>
#include <cstdlib>
#include <magic_enum/magic_enum.hpp>
#include <algorithm>
#include <fstream>
#include <iterator>
#include <sstream>

// The engine's components (FTransformComponent, FTexture2DComponent, ...).
using namespace Components;

DECLARE_LOGGER(Assets, debug)

constexpr float ScreenW = 320.f, ScreenH = 180.f;

//! [generated_texture]
// Builds a texture in code and registers it like a file.
// SDL_PIXELFORMAT_ABGR8888 is the byte order (R, G, B, A) the renderer uploads,
// the same format image files are converted to.
static void RegisterCheckerTexture(const std::string& AssetPath, int Size,
                                   int Squares) {
    SDL_Surface* surface = SDL_CreateSurface(Size, Size, SDL_PIXELFORMAT_ABGR8888);
    const Uint32 light = SDL_MapSurfaceRGBA(surface, 235, 235, 240, 255);
    const Uint32 dark = SDL_MapSurfaceRGBA(surface, 70, 80, 110, 255);
    const int cell = Size / Squares;
    for (int y = 0; y < Squares; ++y) {
        for (int x = 0; x < Squares; ++x) {
            const bool bLight = (x + y) % 2 == 0;
            const SDL_Rect rect {x * cell, y * cell, cell, cell};
            SDL_FillSurfaceRect(surface, &rect, bLight ? light : dark);
        }
    }

    TObjectPtr<Graphics::STexture2D> texture;   // TObjectPtr<T>() creates a new T
    texture->Surface = surface;
    texture->Width = static_cast<float>(Size);
    texture->Height = static_cast<float>(Size);
    texture->AssetPath = AssetPath;

    TObjectPtr<Filesystem::SAssetData> assetData;
    assetData->AssetPath = AssetPath;           // the key GetAssetData() looks up
    assetData->AssetName = AssetPath;
    assetData->AssetType = Filesystem::TEXTURE;
    assetData->ObjectData = texture;            // already loaded: LoadFile() returns it
    Filesystem::RegisterAssetData(assetData);
}
//! [generated_texture]

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

//! [asset_browser]
// An editor window that lists every registered asset, with a filter box.
class AssetBrowser : public ImGUIWrapper::IEditorTool {
protected:
    void Draw_Internal() override {
        ImGui::SetWindowCollapsed("Dear ImGui Demo", true, ImGuiCond_Once);
        ImGui::SetNextWindowSize({560.f, 300.f}, ImGuiCond_FirstUseEver);
        ImGui::Begin("Asset Browser", &bOpen);

        static std::string filter;
        // std::string overload from imgui_stdlib.h
        ImGui::InputText("Filter", &filter);
        for (const std::string& path : Filesystem::GetAllAssetPaths()) {
            if (!filter.empty() && path.find(filter) == std::string::npos) {
                continue;
            }
            const auto data = Filesystem::GetAssetData(path);
            const std::string type {magic_enum::enum_name(data->AssetType)};
            const char* state = data->ObjectData.IsValid() ? "loaded" : "";
            ImGui::Text("%-48s %-8s %s", path.c_str(), type.c_str(), state);
        }
        ImGui::End();
    }
};
//! [asset_browser]

//! [memory_types]
// Any type can live in a TObjectPtr.
// Types derived from SAsset can also be loaded by the Filesystem module.
struct SInventory {
    std::vector<std::string> Items;
};
//! [memory_types]

//! [custom_asset]
// A custom asset type: derive from SAsset, then give LoadAsset<T> a specialization
// that builds one from the file. DEFINE_ASSET_LOADER declares the specialization
// (put it in a header if other files use it).
struct SLineCountAsset : SAsset {
    size_t Lines = 0;
};
DEFINE_ASSET_LOADER(SLineCountAsset, )

namespace Filesystem::Importers {
    template<>
    TObjectPtr<SLineCountAsset> LoadAsset<SLineCountAsset>(
            const TObjectPtr<SAssetData>& InAssetData) {
        std::ifstream file {InAssetData->AssetFilePathRaw};
        const std::istreambuf_iterator<char> begin(file), end;
        TObjectPtr<SLineCountAsset> asset;
        asset->Lines = std::count(begin, end, '\n');
        return asset;
    }
}
//! [custom_asset]

int main() {
    RegisterSolidTexture("CollisionTexture", 8);
    RegisterSolidTexture("White", 8);

    Engine::InitializePlugins();   // scans Resources/ and registers every file it finds
    flecs::world& world = ECS::GetWorld();

    //! [listing]
    // Every file under Resources/ has an asset path:
    // its path relative to Resources/, without the extension.
    const std::vector<std::string> allPaths = Filesystem::GetAllAssetPaths();
    Log::Assets::Info("{} assets registered", allPaths.size());

    // Asset types come from Resources/Config/filesystem.json (file extension -> type).
    for (const auto& shader : Filesystem::GetAllAssetPaths(Filesystem::SHADER)) {
        Log::Assets::Info("Shader asset: {}", shader.AssetPath);
    }
    //! [listing]

    //! [asset_data]
    // GetAssetData accepts '/' or '\' separators.
    // It logs an error and returns null if nothing matches.
    const TObjectPtr<Filesystem::SAssetData> data =
        Filesystem::GetAssetData("Shaders/Compiled/PullSpriteBatch.vert");
    if (data.IsValid()) {
        Log::Assets::Info("Name: {}  Extension: {}  File: {}",
                          data->AssetName, data->AssetExtension,
                          data->AssetFilePathRaw.string());
    }
    //! [asset_data]

    //! [load_text]
    // LoadFile<T> reads the file once and caches the result on the asset's SAssetData.
    // The file is Resources/Shaders/FragmentShaderSrc.frag.hlsl
    const std::string shaderPath = "Shaders/FragmentShaderSrc.frag";
    if (Filesystem::Importers::AssetExists<STextFileAsset>(shaderPath)) {
        TObjectPtr<STextFileAsset> source =
            Filesystem::Importers::LoadFile<STextFileAsset>(shaderPath);
        std::istringstream lines(source->Data);
        std::string firstLine;
        std::getline(lines, firstLine);
        Log::Assets::Info("{} is {} characters long. First line: {}",
                          source->GetName(), source->Data.size(), firstLine);

        // Loading again returns the same object from the cache.
        TObjectPtr<STextFileAsset> again =
            Filesystem::Importers::LoadFile<STextFileAsset>(shaderPath);
        Log::Assets::Info("Cached: {}", again == source);
    }
    //! [load_text]

    //! [load_custom]
    // LoadFile works the same for your own types. Note that an asset is cached for
    // the first type it is loaded as, so don't load one file as two different types.
    const std::string vertexPath = "Shaders/VertexShaderSrc.vert";
    TObjectPtr<SLineCountAsset> vertexSource =
        Filesystem::Importers::LoadFile<SLineCountAsset>(vertexPath);
    Log::Assets::Info("{} has {} lines", vertexSource->GetName(), vertexSource->Lines);
    //! [load_custom]

    //! [register_runtime]
    // Assets can be registered at any time.
    // The renderer uploads a texture the first time it is drawn.
    // Asset paths use the platform's preferred separator ('\' on Windows), like
    // the paths of scanned files. GetAssetData() converts '/' for you, but
    // Filesystem::IsValid() (used by the renderer) does not.
    std::filesystem::path checkerFile = "Generated/Checker";
    const std::string checkerPath = checkerFile.make_preferred().string();
    RegisterCheckerTexture(checkerPath, 64, 8);
    const bool bValid = Filesystem::IsValid(Filesystem::SSoftAssetPath(checkerPath));
    Log::Assets::Info("{} valid: {}", checkerPath, bValid);
    //! [register_runtime]

    //! [object_ptr]
    // TObjectPtr<T> is a reference-counted pointer.
    // Copies share the object; the last one deletes it.
    TObjectPtr<SInventory> inventory;                  // creates a new SInventory
    inventory->Items = {"sword", "shield"};
    TObjectPtr<SInventory> sameInventory = inventory;  // a second reference to it
    sameInventory->Items.push_back("potion");
    Log::Assets::Info("Items: {} (both pointers see {})",
                      inventory->Items.size(), sameInventory->Items.size());

    TObjectPtr<SInventory> empty = nullptr;            // an empty pointer
    Log::Assets::Info("empty.IsValid() = {}", empty.IsValid());

    sameInventory.Clear();                             // drop one reference
    Log::Assets::Info("After Clear: inventory valid = {}, sameInventory valid = {}",
                      inventory.IsValid(), sameInventory.IsValid());

    // CastTo converts between related types
    // (a static_cast: the object must really be of that type).
    TObjectPtr<STextFileAsset> text;
    text->Data = "hello";
    TObjectPtr<SAsset> asBase = text;                  // derived -> base is implicit
    TObjectPtr<STextFileAsset> asText = CastTo<STextFileAsset>(asBase);
    Log::Assets::Info("CastTo round trip: {}", asText->Data);
    //! [object_ptr]

    CreateCameraAndBackground(world, {0.12f, 0.12f, 0.14f, 1.f});
    world.entity("Checkerboard")
        .set<FTransformComponent>({ .Location = {120.f, 50.f, 0.f} })
        .set<FTexture2DComponent>({
            .Sprite = { .SpriteSheet = {checkerPath} },
            .TextureSize = {80.f, 80.f},
        })
        .set<FRenderingComponent>({ .RenderLayer = Renderer::Entities });

    CoreEditor::Open<AssetBrowser>();
    Log::Assets::Info("Close the window to quit.");

    Engine::Run();
    Engine::TeardownPlugins();
    // Known issue: avoid the crash in static destructors after teardown.
    std::quick_exit(0);
}
