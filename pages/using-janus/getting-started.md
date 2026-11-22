# Installing and building {#getting_started}

<p class="lede">What to install, how the engine builds, and how to compile and run the examples. By the end you will have a working engine and thirteen example programs on your machine.</p>

## What you need

| Tool | Version | Why |
| --- | --- | --- |
| Windows | 10 or 11, 64-bit | The engine has only been built and run on Windows. The build copies a DLL from the `x64-windows` vcpkg folder (\src{CMakeLists.txt,75,81}). |
| Visual Studio (MSVC) | A version with C++26 support. These docs were verified with MSVC 14.51 (Visual Studio 2026). | The engine sets `CMAKE_CXX_STANDARD 26` (\src{CMakeLists.txt,8}). |
| CMake | 4.2 or newer | `cmake_minimum_required(VERSION 4.2)` (\src{CMakeLists.txt,1}). |
| Ninja | Any recent version | Recommended generator. Visual Studio's own generator also works. |
| vcpkg | Recent | Installs every third-party library from \src{vcpkg.json}. |
| Git | Any | To get the code. |
| A GPU with Vulkan drivers | Vulkan 1.x | The renderer asks SDL for a Vulkan GPU device (\src{src/Rendering/Vulkan_Renderer.cpp,476}). |

The Vulkan SDK is **not** needed to build or run: vcpkg builds the Vulkan headers and loader. You only need it to recompile the shaders in `Resources/Shaders` (see \src{Tools/CompileShaders.sh}).

### Libraries the engine uses

vcpkg installs these for you. All of them are linked publicly, so your game can use them too (\src{CMakeLists.txt,52,69}).

| Library | Used for |
| --- | --- |
| [SDL3](https://wiki.libsdl.org/SDL3), SDL3_image, SDL3_shadercross | Window, events, GPU rendering, image loading, shader loading |
| [flecs](https://www.flecs.dev/flecs/) | The entity component system |
| [Dear ImGui](https://github.com/ocornut/imgui) (docking branch), ImGuizmo | Editor tools and in-game UI |
| [spdlog](https://github.com/gabime/spdlog) | Logging |
| [glm](https://github.com/g-truc/glm) | Vectors and matrices |
| [nlohmann/json](https://github.com/nlohmann/json), magic_enum | Config files, LDtk levels, enum names |
| [Tracy](https://github.com/wolfpld/tracy) | Profiling (Debug builds) |
| box2d, Boost, GLEW/OpenGL, Vulkan Memory Allocator | Linked but not used by the engine code at this commit |

## Get the code

The engine lives in its own repository, [RPG-Engine-Core](https://github.com/RPG-Engine/RPG-Engine-Core). Games include it as a subfolder, usually named `Engine/` (this is how ProtoRPG is set up):

```
git clone https://github.com/RPG-Engine/RPG-Engine-Core.git Engine
```

## Set up vcpkg

1. Clone and bootstrap vcpkg once, anywhere on your machine:
   ```
   git clone https://github.com/microsoft/vcpkg.git C:\vcpkg
   C:\vcpkg\bootstrap-vcpkg.bat
   ```
2. Pass vcpkg's toolchain file to CMake when you configure: `-DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake`.

vcpkg runs in *manifest mode*: on the first configure it reads `vcpkg.json` and builds every dependency. This takes a while (30 minutes or more the first time); later configures reuse the binaries.

\note vcpkg reads the `vcpkg.json` in the **top-level** source folder. When the engine is a subfolder of your game, either copy `Engine/vcpkg.json` next to your game's `CMakeLists.txt` (ProtoRPG does this), or point vcpkg at the engine's manifest with `-DVCPKG_MANIFEST_DIR=<path to Engine>`.

## Build from a developer prompt

Ninja needs the MSVC compiler on `PATH`. Open **x64 Native Tools Command Prompt for VS**, or run `vcvars64.bat` from `<Visual Studio>\VC\Auxiliary\Build` first. IDEs that understand CMake (Visual Studio's *Open Folder*, CLion, VS Code with CMake Tools) do this for you.

### Build the engine on its own

This is a good first check that your tools work:

```
cmake -S Engine -B Engine/build -G Ninja -DCMAKE_BUILD_TYPE=Debug ^
      -DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake
cmake --build Engine/build
```

The result is the static library `Engine.lib`. The build also:

- generates `Build/Generated/EngineExport.h` inside the engine folder (\src{cmake_modules/CreateLibrary.cmake,24,28}). It defines `ENGINE_EXPORT`, which is empty for this static library.
- copies the engine's `Config/` and `Resources/` folders next to the output folder (\src{cmake_modules/MoveConfigFiles.cmake,9,24}). `Resources/Shaders` holds the compiled shaders the renderer needs.

\warning **Commit 824ee20 does not compile as-is.** \src{include/Module.hpp,17,18} uses `ENGINE_ENGINE_EXPORT`, which is not defined. Replace both with `ENGINE_EXPORT`. See \ref known_issues.

### Build and run the examples {#getting_started_examples}

The examples have their own CMake project in `Engine/Documentation/examples`. It adds the engine as a subdirectory and builds one executable per example:

```
cmake -S Engine/Documentation/examples -B build-examples -G Ninja -DCMAKE_BUILD_TYPE=Release ^
      -DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake ^
      -DVCPKG_MANIFEST_DIR=Engine
cmake --build build-examples
```

Every executable lands in `build-examples/bin`, together with the engine's `Resources/` folder and a `Resources/Config/filesystem.json` the examples project puts there. **Run the examples from that folder**, because the engine reads `Resources/` from the current working directory:

```
cd build-examples\bin
DodgeGame.exe
```

| Executable | Example |
| --- | --- |
| `DodgeGame`, `SnakeGame`, `BreakoutGame` | \ref examples_games |
| `EcsMovement`, `EcsSpawning`, `EcsQueries`, `EcsCollision` | \ref examples_ecs |
| `HelloJanus`, `LoggingExample`, `InputExample`, `AssetsExample`, `RenderingExample`, `EditorExample` | \ref examples_modules |

## Debug and Release builds

| | Debug | Release |
| --- | --- | --- |
| Defines (\src{CMakeLists.txt,83,86}) | `WITH_DEBUG`, `TRACY_ENABLE` | `WITH_RELEASE` |
| Tracy profiler | On: connect the [Tracy profiler](https://github.com/wolfpld/tracy) to see frame timings and engine zones | Off |
| Speed | Much slower (flecs, ImGui and the STL are unoptimized) | Use this to judge performance |

\note The first time you run a Debug build, Windows may ask whether to allow the program on your network. That is Tracy listening for a profiler connection. You can deny it; the game runs either way.

## If it doesn't work

| Symptom | Cause and fix |
| --- | --- |
| `'ENGINE_ENGINE_EXPORT': undeclared identifier` or a syntax error in `Module.hpp` | See the warning above: use `ENGINE_EXPORT`. |
| CMake: *Compatibility with CMake < 4.2* or similar | Your CMake is too old. CLion and Visual Studio bundle their own; check which one runs. |
| `filesystem_error` ... `Resources\Config` when the game starts | The working directory has no `Resources/Config` folder. Run from the output folder, and see \ref essentials_resources. |
| `Asset CollisionTexture could not be loaded` | The renderer needs a texture named `CollisionTexture`. See \ref essentials_minimum. |
| The window opens but stays black | No light, or no camera. See \ref essentials_minimum. |
| Moving sprites leave trails | Nothing on the Background layer clears the screen. See \ref essentials_minimum. |
| The program exits with code `0xC0000005` after closing the window | A known crash after shutdown. See \ref known_issues_shutdown. |
| Copying `TracyClient.dll` fails after building | The post-build step expects the `x64-windows` vcpkg triplet (\src{CMakeLists.txt,75,81}). |

Next: \ref first_game.
