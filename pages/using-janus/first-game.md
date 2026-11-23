# Your first game {#first_game}

<p class="lede">Set up a game project next to the engine, build it, and put a sprite on screen. The code is the \ref ex_hello example; the project files below were built and run against the documented commit.</p>

## 1. Lay out the project

Create a folder for your game with the engine inside it. This is the same layout as ProtoRPG:

```
MyGame/
├── CMakeLists.txt
├── vcpkg.json          copy of Engine/vcpkg.json
├── Engine/             the engine repository
├── Resources/          your assets (images, levels, ...)
├── Config/             project config files
└── Game/
    └── src/
        └── main.cpp
```

- `vcpkg.json` must be in the top-level folder so vcpkg installs the engine's dependencies (\ref getting_started).
- `Game/src/` holds your game's code. Add more `.cpp` files there and list them in `add_executable()`.
- Everything in `Resources/` is copied to `Build/<config>/bin/Resources` after each build, and the engine registers every file in it as an asset.
- `Config/` is created by the build if it is missing and copied to `Build/<config>/bin/Config/Resources` (\src{cmake_modules/MoveConfigFiles.cmake,9,24}). At the documented commit the engine does not read from that folder.
- The engine reads its settings from <b>`Resources/Config`</b> instead. Create that folder and put a copy of `Engine/Config/filesystem.json` in it: the file tells the engine which file extensions are textures, shaders, and so on. The folder must exist even if it is empty, or the engine stops at startup (see \ref essentials_resources).

## 2. Write CMakeLists.txt

```cmake
cmake_minimum_required(VERSION 4.2)
project(MyGame)                         # must match the executable name below

set(CMAKE_CXX_STANDARD 26)

# Put the engine and the game in one output folder: MyGame/Build/<config>/bin
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/Engine/cmake_modules")
include(DetermineOutputDir)

add_subdirectory(Engine)

add_executable(MyGame Game/src/main.cpp)
target_link_libraries(MyGame PRIVATE Engine)

# After each build, copy MyGame/Resources (and MyGame/Config) next to the executable
include(MoveConfigFiles)
```

This is the same setup as ProtoRPG's root `CMakeLists.txt`. Two engine CMake helpers do the work:

- **DetermineOutputDir** sets `CMAKE_RUNTIME_OUTPUT_DIRECTORY` to `Build/<config>/bin` under your game folder (\src{cmake_modules/DetermineOutputDir.cmake,3,7}). The engine uses the same folder, so its `Resources/Shaders` end up next to your game.
- **MoveConfigFiles** adds a post-build step to the target named like the project, which is why `project()` and `add_executable()` use the same name (\src{cmake_modules/MoveConfigFiles.cmake,9,24}).

## 3. Write main.cpp

Every Janus program follows the same shape:

1. Register anything the engine needs before it starts.
2. Call `Engine::InitializePlugins()`.
3. Create entities and systems.
4. Call `Engine::Run()`, which returns when the window is closed.
5. Call `Engine::TeardownPlugins()`.
6. End with `std::quick_exit(0)` rather than returning, to avoid a known crash after shutdown (\ref known_issues_shutdown).

Here is the complete \ref ex_hello example. It generates its textures in code, so it needs no image files:

\genexopen{modules/HelloJanus.cpp,Hello Janus: a camera\, a light\, a background and one sprite}
\include modules/HelloJanus.cpp
\endex

What each part does:

| Code | Why it is there |
| --- | --- |
| `RegisterSolidTexture("CollisionTexture", 8)` | The renderer loads a texture called `CollisionTexture` during startup, for drawing debug colliders. Without it, startup fails. A file `Resources/CollisionTexture.png` works too. |
| `RegisterSolidTexture("White", 8)` | A plain white texture. Sprites tint it with their `Color`, so one texture can draw any coloured rectangle. |
| `Engine::InitializePlugins()` | Starts SDL and every engine module, opens the window, and scans `Resources/`. See \ref engine_core. |
| Camera entity | `FCameraComponent` decides which part of the world is visible. Without a camera nothing is drawn. |
| `FLightComponent` | The final image is lit by lights. With no light at all the screen stays black. |
| Background entity | A sprite on the `Background` layer covering the view. Drawing the Background layer is what clears the screen each frame. |
| Square entity | A sprite: `FTransformComponent` (where), `FTexture2DComponent` (what), `FRenderingComponent` (which layer). |
| `Engine::Run()` | Runs the game loop: input, ECS systems, rendering. See \ref engine_organization. |
| `std::quick_exit(0)` | Ends the program without running static destructors, which currently crash after `Engine::TeardownPlugins()`. |

\ref essentials explains each of these rules in more detail.

## 4. Build and run

```
cmake -S MyGame -B MyGame/build -G Ninja -DCMAKE_BUILD_TYPE=Debug ^
      -DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake
cmake --build MyGame/build
cd MyGame\Build\Debug\bin
MyGame.exe
```

Run it from `Build/Debug/bin`: the engine finds `Resources/` relative to the current working directory. In an IDE, set the run configuration's working directory to that folder.

You should see a dark blue window with an orange square, plus three Dear ImGui windows from the engine's editor tools: **FPS Counter**, **Entity Debugger** and **Dear ImGui Demo**. Open *Entity Debugger → Entity Outliner* and click `Square` to inspect and edit its components while the game runs.

\image html hello-janus.png "Hello Janus: the orange square, with the engine's FPS Counter, Entity Debugger and the (collapsible) Dear ImGui demo window"

## 5. Use your own images

Put image files under `Resources/`, for example `MyGame/Resources/Images/Player.png`. After a build it is copied next to the executable, registered at startup, and can be used by its *asset path*: its path under `Resources/`, without the extension, with Windows separators. In C++, that is `"Images\\Player"`:

```cpp
.set<Components::FTexture2DComponent>({
    .Sprite = { .SpriteSheet = {"Images\\Player"} },
    .TextureSize = {32, 32},
})
```

PNG and JPEG files are supported (see `filesystem.json`). The renderer uploads a texture the first time a sprite uses it. See \ref filesystem for the details of asset paths.

## Where to go next

- Make it move: \ref ex_movement shows input, a velocity component and a camera that follows the player.
- Read the rules: \ref essentials.
- Learn the ECS: \ref ecs, then \ref examples_ecs.
- Look at complete games: \ref examples_games.
