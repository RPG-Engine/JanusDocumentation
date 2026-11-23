# Known issues {#known_issues}

<p class="lede">Engine behaviour at commit \docscommit that may surprise you, found while writing and running the examples. Each entry links to the code and says how to work around it.</p>

These are descriptions of the engine as it is, not of how it should be. When one is fixed, remove it here (see \ref docs_version).

## Building and starting

| Issue | Workaround |
| --- | --- |
| If `SDL_Init` fails, `InitializePlugins()` executes `throw;` with no active exception, which terminates the program without a message box (\src{src/Module.cpp,76,79}). | Check the log for the SDL error. |
| The post-build step copies `TracyClient.dll` from the `x64-windows` triplet folder (\src{CMakeLists.txt,75,81}). | Use the `x64-windows` triplet. |

## Shutdown {#known_issues_shutdown}

**Returning from `main()` after Engine::TeardownPlugins() crashes** with an access violation (exit code `0xC0000005`). The crash happens in static destructors, after all logs are written: ECS::TeardownModule() destroys the flecs world with `ecs_fini` (\src{src/Gameplay/ECS/Module.cpp,61,64}), and the same world, a function-local static (\src{src/Gameplay/ECS/Module.cpp,66,69}), is destroyed again when the program exits.

**Workaround:** end `main()` with `std::quick_exit(0)`, as every example does. Ending with `std::_Exit(0)` gives exit code 0 as well.

## Filesystem {#known_issues_filesystem}

| Issue | Workaround |
| --- | --- |
| `Resources/Config` must exist in the working directory; otherwise startup throws `std::filesystem::filesystem_error` (\src{src/Filesystem/Module.cpp,27,29}). | Create it, and put `filesystem.json` in it. |
| The engine's CMake copies `Config/` to `<output>/Config/Resources` instead of `<output>/Resources/Config` (\src{cmake_modules/MoveConfigFiles.cmake,9,16}), so `filesystem.json` is not found and every asset type is `UNDEFINED`. | Put a copy of `filesystem.json` in your game's `Resources/Config/` (\ref first_game). |
| `Could not find asset [Config\filesystem]` is logged once at startup even when the file exists. | Harmless. |
| Filesystem::IsValid() compares paths exactly, while Filesystem::GetAssetData() converts `/` to `\` (\src{src/Filesystem/Module.cpp,67,76}, \src{src/Filesystem/Module.cpp,119,129}). A sprite whose path uses `/` is not drawn on Windows. | Use `\\` in stored paths, or `std::filesystem::path(p).make_preferred()`. |
| Files with the same name and different extensions share one asset path; only the first is registered. | Give files unique names. |
| An asset is cached for the first type it is loaded as; loading it as another type returns a wrongly cast object (\src{include/Filesystem/Importers/Importer.hpp,67,69}). | Load each file as one type. |
| `TryLoadFile` throws `nullptr` instead of returning an empty pointer (\src{include/Filesystem/Importers/Importer.hpp,109}). | Check `AssetExists` or Filesystem::IsValid() first. |
| Filesystem::GetAssetData(const TObjectPtr<SAsset>&) looks the asset up by name, not path (\src{src/Filesystem/Module.cpp,78,81}). | Use `asset->GetAssetData()`. |
| Constructing an STypedSoftAssetPath from an SAssetData of another type runs `throw;` and terminates (\src{include/Filesystem/FilesystemTypes.hpp,153,159}). | Construct it from a path string. |
| `SConfig::Read()` and `SConfig::Save()` don't compile if used; `Get()` re-reads the file on every call because `bIsLoaded` is never set (\src{include/Filesystem/Config.hpp,20,50}). | Use `Get()` only, and cache the result. |
| Filesystem::Module() and `Filesystem::Importers::Load` (LDtk) are declared but not defined. | Don't call them. |

## Rendering {#known_issues_rendering}

| Issue | Workaround |
| --- | --- |
| Without any light, the screen is black: the lit image is only drawn when a light exists (\src{src/Rendering/Vulkan_Renderer.cpp,625,627}). | Always have a light (a large-radius one lights everything). |
| Every texture batch on the `Background` layer clears the screen (the "cleared" flag is captured by value, \src{src/Rendering/Vulkan_Renderer.cpp,730}), so only the last Background texture is visible. If the Background layer is empty, nothing clears the screen and moving sprites smear. | Use exactly one texture on the Background layer, covering the view. |
| Only the `Entities` layer is emptied each frame (\src{src/Rendering/Vulkan_Renderer.cpp,783}). Destroyed entities, or entities moved to another layer, stay drawn on the other layers. | Put dynamic sprites on `Entities`; call Renderer::CleanupEntity() before destroying something on another layer. |
| If the first sprite of a batch has an invalid texture path, the whole batch is skipped (\src{src/Rendering/Vulkan_Renderer.cpp,233,235}). | Make sure texture paths are valid. |
| Sprites with different textures on one layer are not depth-sorted against each other. | Use a sprite sheet so sprites that must overlap correctly share a texture. |
| The window and render target are fixed at 1920 × 1080 (\src{src/Rendering/Vulkan_Renderer.cpp,58}); resizing stretches the image and breaks Renderer::ScreenPosToWorldPos(). | Don't resize the window. |
| The flecs reflection registered for Graphics::SSprite lists members the struct doesn't have (\src{src/Rendering/Module.cpp,55,59}); flecs reports a size mismatch at shutdown. Editing or saving sprites in the Entity Debugger may be wrong. | None needed for drawing. |
| Light `Radius` is in screen space, `Falloff` is ignored, the ambient level is fixed at 0.3, and the light count passed to the shader is a byte count (\src{src/Rendering/Vulkan_Renderer.cpp,680,686}). | Tune `Radius` and `Intensity` by eye. |
| Renderer::AddDebugCollision() (deprecated) reads its rectangle as `(x, y, height, width)` (\src{include/Rendering/Module.hpp,81,96}). Renderer::LoadLevel() adds its collision rectangles starting at ID 0, which can clash with your IDs (\src{src/Rendering/Module.cpp,93,94}). | Write to Renderer::CollisionInstances yourself, with IDs that can't clash. |
| Renderer::CleanupEntity() has no return statement when it succeeds (\src{src/Rendering/RenderingBackend.cpp,62,75}). | Ignore its return value. |

## Input {#known_issues_input}

| Issue | Workaround |
| --- | --- |
| Only the keyboard is read; gamepad buttons in bindings are ignored (\src{src/Input/Module.cpp,100,122}). | — |
| Press bindings fire on keyboard auto-repeat (\src{src/Input/Module.cpp,46,49}). | Track the key's previous state if you need one trigger per press. |
| Multi-key press and release bindings never trigger (\src{src/Input/Module.cpp,65,98}). | Use hold bindings for combinations. |
| Each callback on an action runs once per callback registered on that action (\src{include/Input/Module.hpp,96,107}). | Register at most one callback per action. |
| `MULTIPLY` and `DIVIDE` actions always give zero: values start from zero (\src{include/Input/Module.hpp,71,81}). | Use `ADD` or `OVERRIDE`. |
| Keys typed into ImGui text fields also trigger game actions (\src{src/Input/Module.cpp,44}). | Check `ImGui::GetIO().WantCaptureKeyboard` in your systems if it matters. |

## Editor and ImGui

| Issue | Workaround |
| --- | --- |
| The Dear ImGui demo window is always shown (\src{src/Editor/Module.cpp,44}). | Collapse it; `ImGui::SetWindowCollapsed("Dear ImGui Demo", true, ImGuiCond_Once)` in a tool does it from code. |
| Component panels in the Entity Debugger can't be collapsed (a misplaced parenthesis in \src{include/Gameplay/ECS/Components.hpp,43}). | — |
| *Save Entity* writes to `Resources/Output/ECS/`, which must already exist (\src{src/Editor/EntityEditorTool.cpp,182,191}). | Create the folder. |
| `ImGuiWrapper::TeardownModule()` is never called. | None needed. |

## Gameplay

| Issue | Workaround |
| --- | --- |
| The engine never calls GameplayFramework::Module(). | Call it after Engine::InitializePlugins(). |
| `FMouseState::HoveredEntityId` keeps the last hovered entity when the cursor leaves it (\src{src/Gameplay/Module.cpp,62,65}). | Check `MousePosition` against the entity's box, as \ref ex_collision does. |
| If `FMouseState` is missing from the global entity, the hover system dereferences a null pointer (\src{src/Gameplay/Module.cpp,32,35}). | Only start it with GameplayFramework::ActivateMouseStateSystem(). |

## Memory

| Issue | Workaround |
| --- | --- |
| Copy- and move-assigning a `TObjectPtr` that already holds an object doesn't release that object (\src{include/Memory/Janitors.hpp,95,111}). | `Clear()` before assigning, if it may hold something. |
| `TObjectPtr::operator*` doesn't compile if used (\src{include/Memory/Janitors.hpp,120}). | Use `->` or `Raw()`. |
| `CastTo` on an empty pointer crashes (\src{include/Memory/Janitors.hpp,129}). | Check `IsValid()` first. |

## Logging

| Issue | Workaround |
| --- | --- |
| Trace (`Verbose`) messages are always dropped: Log::Module() sets spdlog's global level to debug (\src{src/Log/Module.cpp,54}). | — |
| Timestamped copies of log files are named with the UTC time (\src{src/Log/Module.cpp,63,64}). | — |
