# Engine modules {#modules_overview}

<p class="lede">The engine is one library made of ten modules plus a small core. Each module has its own folder, namespace and page.</p>

Each module page starts with **How to use**, then explains the details and pitfalls, then lists the module's classes and functions. Folders are relative to `include/` (headers) and `src/` (sources).

| Module | What it does | Folder | Started by the engine | Page |
| --- | --- | --- | --- | --- |
| Engine core | Starts the modules, runs the game loop, shuts down | `Module.hpp` | — | \ref engine_core |
| Log | Named loggers built on spdlog | `Log/` | 1st | \ref logging |
| Filesystem | Registry of every file under `Resources/`, typed loading, custom asset types | `Filesystem/` | 2nd | \ref filesystem |
| ImGuiWrapper | The shared Dear ImGui context and debug-panel registry | `Editor/ImGuiWrapper/` | 3rd | \ref imguiwrapper |
| ECS | The flecs world, component macros, built-in components, phases | `Gameplay/ECS/` | 4th | \ref ecs |
| Renderer | Sprites, layers, camera, lights, debug colliders, levels | `Rendering/` | 5th | \ref rendering |
| CoreEditor | Editor tools: Entity Debugger, FPS Counter, your own tools and game UI | `Editor/` | 6th | \ref editor |
| Memory | `TObjectPtr<T>`, the reference-counted pointer | `Memory/` | not needed | \ref memory |
| CoreTypes | Textures, sprites, LDtk levels, frame timing | `Types/` | not needed | \ref coretypes |
| Input | Actions with press, hold and release bindings | `Input/` | not needed (updated every frame by the loop) | \ref input |
| GameplayFramework | Colliders, mouse hover, the `Move2D` action | `Gameplay/` | **no: call `GameplayFramework::Module()` yourself** | \ref gameplay |

## Which module do I need?

| I want to… | Read |
| --- | --- |
| Start the engine and run my game | \ref engine_core, \ref first_game |
| Create game objects and give them behaviour | \ref ecs |
| Draw sprites, move the camera, add lights | \ref rendering |
| React to the keyboard | \ref input |
| Load images, text or my own file types | \ref filesystem |
| Show a score, a menu or a debug window | \ref editor |
| Detect overlaps or what the mouse is over | \ref gameplay |
| Load a level made in LDtk | \ref coretypes, \ref rendering |
| Print messages to the console and log files | \ref logging |
| Share objects safely between systems | \ref memory |

\ref engine_organization shows the start-up order and how the modules depend on each other.
