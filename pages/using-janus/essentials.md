# Things to know {#essentials}

<p class="lede">The rules every Janus game follows. Each one was checked by running the engine; the source links show where the behaviour comes from.</p>

## The minimum for a visible game {#essentials_minimum}

The renderer draws nothing useful until four things exist. Every example sets them up the same way (\ref ex_hello shows the shortest version):

| You need | Because | Without it |
| --- | --- | --- |
| A texture asset named `CollisionTexture`, before `Engine::InitializePlugins()` | The renderer loads it at startup for the debug-collider overlay (\src{src/Rendering/Vulkan_Renderer.cpp,427,429}). | Startup throws `AssetCouldNotLoadException`. |
| An entity with `FTransformComponent` + `FCameraComponent` | The camera system builds the view and projection from it (\src{src/Rendering/Vulkan_Renderer.cpp,293,315}). | Nothing is visible. |
| At least one entity with `FTransformComponent` + `FLightComponent` | The lit scene is copied to the window only when there is a light (\src{src/Rendering/Vulkan_Renderer.cpp,625,627}). | The window stays black (ImGui still shows). |
| A sprite on the `Background` layer that covers the view | Drawing the Background layer is what clears the screen each frame (\src{src/Rendering/Vulkan_Renderer.cpp,726,739}). | Moving sprites smear across the screen. |

`CollisionTexture` can be a file (`Resources/CollisionTexture.png`, as in ProtoRPG) or a texture registered in code, as the examples do:

\genexopen{modules/HelloJanus.cpp,Solid texture}
\snippet modules/HelloJanus.cpp solid_texture
\endex

A light with a very large radius lights the whole screen evenly. `Radius = 100, Intensity = 2.4` gives roughly full brightness; see \ref rendering_lights.

## Startup, the loop and shutdown

```
main()
 ├─ register CollisionTexture (and anything else needed during startup)
 ├─ Engine::InitializePlugins()   SDL, Log, Filesystem, ImGui, ECS,
 │                                Renderer (opens the window), Editor
 ├─ create entities, systems, input bindings, editor tools
 ├─ Engine::Run()                 every frame: input → ECS systems → render,
 │                                until the window closes
 └─ Engine::TeardownPlugins()
```

- `Engine::Run()` caps the loop at 500 frames per second (\src{src/Module.cpp,34}).
- The time since the last frame is passed to flecs, so `it.delta_time()` in a system is in **seconds**.
- The engine never calls `GameplayFramework::Module()`. Call it yourself before using `FColliderComponent` or `FMouseState` (\ref gameplay).
- Returning from `main()` after `Engine::TeardownPlugins()` currently crashes. The examples end with `std::quick_exit(0)`; see \ref known_issues_shutdown.

\ref engine_organization has the full picture.

## Files and the working directory {#essentials_resources}

- The engine reads assets from **`<current working directory>/Resources`**, not from the executable's folder (\src{src/Filesystem/Module.cpp,111,113}). Start your game from its output folder, or set the working directory in your IDE.
- At startup it scans `Resources/Config` first, then all of `Resources/`, and registers every file (\src{src/Filesystem/Module.cpp,24,50}). **`Resources/Config` must exist**, or startup throws a `std::filesystem::filesystem_error`.
- Put `filesystem.json` (from `Engine/Config`) in `Resources/Config`. It maps file extensions to asset types. Without it every asset's type is `UNDEFINED`. Loading by path still works, but editor drop-downs that filter by type stay empty.
- An asset is found by its *asset path*: the file's path under `Resources/` without the extension, with the platform's separators. `Resources/Images/Player.png` is `"Images\\Player"` on Windows. Use `\\` in component values: `Filesystem::IsValid()` and the renderer compare paths exactly (\src{src/Filesystem/Module.cpp,119,129}).
- Log files go to `<executable folder>/Logs` (\src{src/Log/Module.cpp,35}).

\ref filesystem explains asset paths, loading and registering assets made in code.

## World coordinates

- Positions are in **world units**. The camera's `CameraSize` says how many units fit on screen: `{320, 180}` shows a 320 × 180 area.
- **X grows to the right, Y grows down**; `(0, 0)` is the top-left of the view for a camera that does not move.
- A sprite's `FTransformComponent::Location` is its **top-left corner**; `FTexture2DComponent::TextureSize` is its size in world units.
- `Rotation.x` rotates a sprite around its centre, in radians. `Scale` is not used by the renderer.
- The window is 1920 × 1080 and the image is rendered at that size (\src{src/Rendering/Vulkan_Renderer.cpp,58}). Resizing the window stretches it, and `Renderer::ScreenPosToWorldPos` assumes 1920 × 1080.

## Layers and draw order

Sprites are drawn per layer, in this order: `Background`, `Entities`, `Foreground`, `Debug`, `Overlay`. Inside a layer, sprites are grouped by texture, and each texture's sprites are sorted by Y so lower sprites cover higher ones.

| Layer | Use it for | Note |
| --- | --- | --- |
| `Background` | The backdrop and static scenery | Clears the screen. Use **one** texture on this layer: each texture group on it clears again. |
| `Entities` | Anything that moves or gets destroyed | Emptied and rebuilt every frame (\src{src/Rendering/Vulkan_Renderer.cpp,783}). |
| `Foreground`, `Debug`, `Overlay` | Static things drawn on top | **Not** emptied each frame: a destroyed entity stays visible here. |

Details and examples: \ref rendering.

## Systems and phases

- Systems you add with `ECS::GetWorld().system<...>()` run in flecs' `OnUpdate` phase unless you choose another with `.kind(...)`.
- flecs runs its phases in order: `OnLoad`, `PostLoad`, `PreUpdate`, `OnUpdate`, `OnValidate`, `PostUpdate`, `PreStore`, `OnStore`. The engine adds `ECS::Phase::OnInput()` and `ECS::Phase::OnCollision`, which run after `PostUpdate`, and `ECS::Phase::OnPreRender()`, which runs after `OnStore`. The renderer collects sprites, lights and the camera in `OnPreRender`, so anything that changes them should run before it.
- Input has already been read when systems run, so systems can poll actions directly.
- Creating or destroying entities inside a system is deferred until the system finishes, which makes it safe while iterating.

See \ref ecs.

## Input

- Only the **keyboard** is read. Gamepad buttons can be bound but never trigger (\src{src/Input/Module.cpp,100,122}).
- Press bindings also fire on keyboard auto-repeat while a key is held.
- Bind keys to *actions* you declare with `DEFINE_INPUT_ACTION`, then poll them in systems with `Input::GetValue` or `Input::IsActionTriggered`.
- Game input is read even while you type in an ImGui text field.

See \ref input.

## The editor windows

`Engine::InitializePlugins()` opens three Dear ImGui windows: **Entity Debugger** (browse entities, edit components), **FPS Counter**, and the **Dear ImGui Demo**, which the engine always shows (\src{src/Editor/Module.cpp,44}). Collapse the demo with the ▼ in its title bar. ImGui saves window positions in `imgui.ini` next to the executable. The examples collapse it from code:

```cpp
ImGui::SetWindowCollapsed("Dear ImGui Demo", true, ImGuiCond_Once);
```

Multi-viewport is on: an ImGui window dragged outside the game window becomes its own OS window. Game UI (score, menus) is made with editor tools too: see \ref editor.

## Build configurations

Debug builds enable the Tracy profiler (and may trigger a Windows Firewall prompt the first time). Use Release builds to judge performance. See \ref getting_started.
