# How the engine is organized {#engine_organization}

<p class="lede">How the engine is built, how its modules start and stop, and how they depend on each other.</p>

## One library, many modules

The engine is a single static library, the CMake target `Engine` (\src{CMakeLists.txt,26}). Inside it, each engine system is a **module**: a folder of public headers under `include/` and matching sources under `src/`, with its own C++ namespace.

| Part | Where | What it is |
| --- | --- | --- |
| **Engine core** | \src{include/Module.hpp} · \src{src/Module.cpp} | Starts and stops the modules, and runs the game loop (\srclink{src/Module.cpp,30,Engine::Run()}). See \ref engine_core. |
| **Modules** | `include/<Module>/` · `src/<Module>/` | Ten engine systems, e.g. \ref logging, Filesystem and the renderer. See \ref modules_overview. |
| **Config** | \src{Config/filesystem.json} | Settings files, read by the Filesystem module. |

Include a module through its folder, e.g. `#include <Log/Module.hpp>`. Most modules have a `Module.hpp` that pulls in the rest of their public API.

## Startup and shutdown

The engine core calls each module's init function directly, in a fixed order written in \srclink{src/Module.cpp,73,Engine::InitializePlugins()}. At shutdown, \srclink{src/Module.cpp,95,Engine::TeardownPlugins()} tears the modules down in roughly reverse order.

\dot
digraph lifecycle {
  rankdir=TB;
  bgcolor="transparent";
  newrank=true;
  graph [fontname="Helvetica", fontsize=11, nodesep=0.6, ranksep=0.18];
  node [shape=box, style="rounded,filled", fontname="Helvetica", fontsize=10, width=2.2, margin="0.10,0.05"];
  edge [color="#7d8a96", arrowsize=0.7];

  subgraph cluster_start {
    label="Engine::InitializePlugins()"; fontcolor="#1b2733"; color="#2f7fc1"; style="rounded";
    node [fillcolor="#eef4fa", color="#2f7fc1", fontcolor="#1b2733"];
    s0 [label="SDL_Init()"];
    s1 [label="1 · Log::Module()", URL="\ref Log::Module()"];
    s2 [label="2 · Filesystem::InitializeAllResources()"];
    s3 [label="3 · ImGuiWrapper::Module()"];
    s4 [label="4 · ECS::Module()"];
    s5 [label="5 · Renderer::Module()"];
    s6 [label="6 · CoreEditor::ModuleStartup()"];
    s0 -> s1 -> s2 -> s3 -> s4 -> s5 -> s6;
  }

  run [label="Engine::Run()\ngame loop", fillcolor="#e8f6ee", color="#1a7f37", fontcolor="#1b2733"];

  subgraph cluster_stop {
    label="Engine::TeardownPlugins()"; fontcolor="#1b2733"; color="#8c959f"; style="rounded";
    node [fillcolor="#f6f8fa", color="#8c959f", fontcolor="#1b2733"];
    t1 [label="CoreEditor::ModuleTeardown()"];
    t2 [label="Renderer::TeardownModule()"];
    t3 [label="ECS::TeardownModule()"];
    t4 [label="Filesystem::TeardownModule()"];
    t5 [label="Log::TeardownModule()", URL="\ref Log::TeardownModule()"];
    t6 [label="SDL_Quit()"];
    t1 -> t2 -> t3 -> t4 -> t5 -> t6;
  }

  s6 -> run;
  run -> t1;
  { rank=same; s0; t1; }
}
\enddot

<p class="diagram-caption">Startup (left), then the game loop, then shutdown (right). Source: \src{src/Module.cpp,73,105}.</p>

\engex{src/Module.cpp,73,93,Engine::InitializePlugins()}
```cpp
void InitializePlugins() {
    ZoneScoped;

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) == false) {
        Log::Engine::Error(SDL_GetError());
        throw;
    }

    Log::Engine::Info("========= Janus Engine Initializing =========");

    Log::Module();
    Filesystem::InitializeAllResources();
    ImGuiWrapper::Module();
    ECS::Module();
    Renderer::Module();

    CoreEditor::ModuleStartup();


    Log::Engine::Info("Engine initialization complete!");
}
```
\endex

\warning **Four modules are never initialized by the engine.** Memory, CoreTypes, Input and GameplayFramework each have a `Module()` function, but `Engine::InitializePlugins()` does not call any of them. Memory's, CoreTypes' and Input's are currently empty. `GameplayFramework::Module()` registers the `FColliderComponent` and `FMouseState` ECS components (\src{src/Gameplay/Module.cpp,18,23}), so call it yourself if your game uses them. Likewise, `ImGuiWrapper::TeardownModule()` is never called at shutdown.

## The game loop

Once started, \srclink{src/Module.cpp,30,Engine::Run()} repeats these steps every frame until Input reports a quit request:

1. Report the last frame's time (`Engine::Globals::ReportRenderTime_NS`).
2. Read input: `Input::UpdateInputs()`. Stop if `Input::ShouldQuit()`.
3. Update the ECS world: `ECS::UpdateEntities(deltaTime)`.
4. Render: `Renderer::StartRenderFrame()`, `CoreEditor::BeginRender()`, `Renderer::RenderFrame()`, `CoreEditor::EndRender()`.
5. Wait out the rest of the frame. The loop is capped at 500 frames per second.

\engex{src/Module.cpp,40,68,The loop body}
```cpp
while (bTickRunning) {
    ZoneNamedN(Tick, "Engine::Tick", true);
    uint64_t currTime = SDL_GetTicksNS();
    Globals::ReportRenderTime_NS(renderingNs);

    Input::UpdateInputs();

    if (Input::ShouldQuit()) {
        bTickRunning = false;
        break;
    }

    ECS::UpdateEntities(Globals::GetDeltaTime());

    Renderer::StartRenderFrame();
    CoreEditor::BeginRender();

    Renderer::RenderFrame();
    CoreEditor::EndRender();
    renderingNs = SDL_GetTicksNS() - currTime;

    if (renderingNs < nsPerFrame)
    {
        ZoneNamedN(TickWait, "Engine::Tick::Wait", true);
        SDL_DelayNS(nsPerFrame - renderingNs);
        renderingNs =  SDL_GetTicksNS() - currTime;
    }
    FrameMark;
}
```
\endex

## Module dependencies

Modules are listed top to bottom in initialization order. The four modules the engine never initializes are grouped at the bottom. Each arrow goes from a module to another module whose headers it includes. Hover over an arrow to see its name, or click a module to open its folder.

\htmlonly
<svg class="arc-diagram" viewBox="0 0 900 596" role="img" aria-label="Engine modules in initialization order and their include dependencies" xmlns="http://www.w3.org/2000/svg"><defs><marker id="arrow-earlier" viewBox="0 0 10 10" refX="9" refY="5" markerWidth="7" markerHeight="7" orient="auto-start-reverse"><path d="M0,0 L10,5 L0,10 z" class="arc-head-earlier"/></marker><marker id="arrow-later" viewBox="0 0 10 10" refX="9" refY="5" markerWidth="7" markerHeight="7" orient="auto-start-reverse"><path d="M0,0 L10,5 L0,10 z" class="arc-head-later"/></marker></defs><text x="280" y="24" text-anchor="end" class="arc-title">uses a module listed above</text><text x="620" y="24" text-anchor="start" class="arc-title arc-title-later">uses a module the engine never initializes</text><text x="450.0" y="24" text-anchor="middle" class="arc-title">init order ↓</text><line x1="290" y1="363.0" x2="610" y2="363.0" class="arc-sep"/><text x="450.0" y="367.0" text-anchor="middle" class="arc-sep-label">not initialized by Engine::InitializePlugins()</text><path d="M300,327.0 C248,327.0 248,223.0 300,223.0" class="arc-earlier" marker-end="url(#arrow-earlier)"><title>CoreEditor uses ECS</title></path><path d="M300,311.0 C218,311.0 218,127.0 300,127.0" class="arc-earlier" marker-end="url(#arrow-earlier)"><title>CoreEditor uses Filesystem</title></path><path d="M300,319.0 C233,319.0 233,170.6 300,170.6" class="arc-earlier" marker-end="url(#arrow-earlier)"><title>CoreEditor uses ImGuiWrapper</title></path><path d="M300,335.0 C263,335.0 263,279.0 300,279.0" class="arc-earlier" marker-end="url(#arrow-earlier)"><title>CoreEditor uses Renderer</title></path><path d="M600,323.0 C652,323.0 652,449.0 600,449.0" class="arc-later" marker-end="url(#arrow-later)"><title>CoreEditor uses CoreTypes</title></path><path d="M300,211.0 C248,211.0 248,111.0 300,111.0" class="arc-earlier" marker-end="url(#arrow-earlier)"><title>ECS uses Filesystem</title></path><path d="M300,215.0 C263,215.0 263,161.0 300,161.0" class="arc-earlier" marker-end="url(#arrow-earlier)"><title>ECS uses ImGuiWrapper</title></path><path d="M300,545.0 C188,545.0 188,235.0 300,235.0" class="arc-earlier" marker-end="url(#arrow-earlier)"><title>GameplayFramework uses ECS</title></path><path d="M300,537.0 C173,537.0 173,185.0 300,185.0" class="arc-earlier" marker-end="url(#arrow-earlier)"><title>GameplayFramework uses ImGuiWrapper</title></path><path d="M300,561.0 C263,561.0 263,511.0 300,511.0" class="arc-earlier" marker-end="url(#arrow-earlier)"><title>GameplayFramework uses Input</title></path><path d="M300,553.0 C203,553.0 203,285.0 300,285.0" class="arc-earlier" marker-end="url(#arrow-earlier)"><title>GameplayFramework uses Renderer</title></path><path d="M300,499.0 C203,499.0 203,231.0 300,231.0" class="arc-earlier" marker-end="url(#arrow-earlier)"><title>Input uses ECS</title></path><path d="M300,487.0 C188,487.0 188,180.2 300,180.2" class="arc-earlier" marker-end="url(#arrow-earlier)"><title>Input uses ImGuiWrapper</title></path><path d="M300,273.0 C263,273.0 263,219.0 300,219.0" class="arc-earlier" marker-end="url(#arrow-earlier)"><title>Renderer uses ECS</title></path><path d="M300,261.0 C233,261.0 233,119.0 300,119.0" class="arc-earlier" marker-end="url(#arrow-earlier)"><title>Renderer uses Filesystem</title></path><path d="M300,267.0 C248,267.0 248,165.8 300,165.8" class="arc-earlier" marker-end="url(#arrow-earlier)"><title>Renderer uses ImGuiWrapper</title></path><path d="M600,273.0 C667,273.0 667,437.0 600,437.0" class="arc-later" marker-end="url(#arrow-later)"><title>Renderer uses CoreTypes</title></path><path d="M300,461.0 C218,461.0 218,227.0 300,227.0" class="arc-earlier" marker-end="url(#arrow-earlier)"><title>CoreTypes uses ECS</title></path><path d="M300,437.0 C188,437.0 188,135.0 300,135.0" class="arc-earlier" marker-end="url(#arrow-earlier)"><title>CoreTypes uses Filesystem</title></path><path d="M300,449.0 C203,449.0 203,175.4 300,175.4" class="arc-earlier" marker-end="url(#arrow-earlier)"><title>CoreTypes uses ImGuiWrapper</title></path><path d="M600,461.0 C637,461.0 637,499.0 600,499.0" class="arc-later" marker-end="url(#arrow-later)"><title>CoreTypes uses Input</title></path><a href="https://github.com/RPG-Engine/RPG-Engine-Core/tree/02fd6c2454ea67f96d38295400c5e2a88619f43e/include/Log" target="_blank"><g class="arc-node arc-node-foundation"><rect x="300" y="56" width="300" height="34" rx="5"/><text x="314" y="78.0" class="arc-num">1</text><text x="340" y="78.0" class="arc-name">Log</text><text x="590" y="77.0" text-anchor="end" class="arc-note">linked by most modules</text></g></a><a href="https://github.com/RPG-Engine/RPG-Engine-Core/tree/02fd6c2454ea67f96d38295400c5e2a88619f43e/include/Filesystem" target="_blank"><g class="arc-node"><rect x="300" y="106" width="300" height="34" rx="5"/><text x="314" y="128.0" class="arc-num">2</text><text x="340" y="128.0" class="arc-name">Filesystem</text></g></a><a href="https://github.com/RPG-Engine/RPG-Engine-Core/tree/02fd6c2454ea67f96d38295400c5e2a88619f43e/include/Editor/ImGuiWrapper" target="_blank"><g class="arc-node"><rect x="300" y="156" width="300" height="34" rx="5"/><text x="314" y="178.0" class="arc-num">3</text><text x="340" y="178.0" class="arc-name">ImGuiWrapper</text></g></a><a href="https://github.com/RPG-Engine/RPG-Engine-Core/tree/02fd6c2454ea67f96d38295400c5e2a88619f43e/include/Gameplay/ECS" target="_blank"><g class="arc-node"><rect x="300" y="206" width="300" height="34" rx="5"/><text x="314" y="228.0" class="arc-num">4</text><text x="340" y="228.0" class="arc-name">ECS</text></g></a><a href="https://github.com/RPG-Engine/RPG-Engine-Core/tree/02fd6c2454ea67f96d38295400c5e2a88619f43e/include/Rendering" target="_blank"><g class="arc-node"><rect x="300" y="256" width="300" height="34" rx="5"/><text x="314" y="278.0" class="arc-num">5</text><text x="340" y="278.0" class="arc-name">Renderer</text></g></a><a href="https://github.com/RPG-Engine/RPG-Engine-Core/tree/02fd6c2454ea67f96d38295400c5e2a88619f43e/include/Editor" target="_blank"><g class="arc-node"><rect x="300" y="306" width="300" height="34" rx="5"/><text x="314" y="328.0" class="arc-num">6</text><text x="340" y="328.0" class="arc-name">CoreEditor</text></g></a><a href="https://github.com/RPG-Engine/RPG-Engine-Core/tree/02fd6c2454ea67f96d38295400c5e2a88619f43e/include/Memory" target="_blank"><g class="arc-node arc-node-foundation arc-node-uninit"><rect x="300" y="382" width="300" height="34" rx="5"/><text x="314" y="404.0" class="arc-num">–</text><text x="340" y="404.0" class="arc-name">Memory</text><text x="590" y="403.0" text-anchor="end" class="arc-note">linked by most modules</text></g></a><a href="https://github.com/RPG-Engine/RPG-Engine-Core/tree/02fd6c2454ea67f96d38295400c5e2a88619f43e/include/Types" target="_blank"><g class="arc-node arc-node-uninit"><rect x="300" y="432" width="300" height="34" rx="5"/><text x="314" y="454.0" class="arc-num">–</text><text x="340" y="454.0" class="arc-name">CoreTypes</text><text x="590" y="453.0" text-anchor="end" class="arc-note">never initialized</text></g></a><a href="https://github.com/RPG-Engine/RPG-Engine-Core/tree/02fd6c2454ea67f96d38295400c5e2a88619f43e/include/Input" target="_blank"><g class="arc-node arc-node-uninit"><rect x="300" y="482" width="300" height="34" rx="5"/><text x="314" y="504.0" class="arc-num">–</text><text x="340" y="504.0" class="arc-name">Input</text><text x="590" y="503.0" text-anchor="end" class="arc-note">never initialized</text></g></a><a href="https://github.com/RPG-Engine/RPG-Engine-Core/tree/02fd6c2454ea67f96d38295400c5e2a88619f43e/include/Gameplay" target="_blank"><g class="arc-node arc-node-uninit"><rect x="300" y="532" width="300" height="34" rx="5"/><text x="314" y="554.0" class="arc-num">–</text><text x="340" y="554.0" class="arc-name">GameplayFramework</text><text x="590" y="553.0" text-anchor="end" class="arc-note">never initialized</text></g></a></svg>
\endhtmlonly

<p class="diagram-caption"><span class="legend"></span>uses a module listed above &nbsp;&nbsp; <span class="legend later"></span>uses a module the engine never initializes</p>

To keep the diagram readable, it leaves out links to **Log** and **Memory**, which most modules use. The table below lists every link. Every module also builds into the same library, so these are source dependencies, not separate binaries.

| Module | Uses (by `#include`) | First include |
| --- | --- | --- |
| Log | none | |
| Memory | none | |
| Filesystem | Log, Memory | \src{include/Filesystem/Module.hpp,5} |
| ImGuiWrapper | Memory | \src{include/Editor/ImGuiWrapper/Module.hpp,12} |
| ECS | Filesystem, ImGuiWrapper, Log | \src{include/Gameplay/ECS/Components.hpp,13} |
| Renderer | ECS, Filesystem, ImGuiWrapper, Log, Memory, CoreTypes | \src{include/Rendering/Module.hpp,5} |
| CoreEditor | ECS, Filesystem, ImGuiWrapper, Memory, Renderer, CoreTypes | \src{include/Editor/Module.hpp,10} |
| CoreTypes | ECS, Filesystem, ImGuiWrapper, Input, Log, Memory | \src{include/Types/Graphics.hpp,11} |
| Input | ECS, ImGuiWrapper, Log, Memory | \src{include/Input/Module.hpp,4} |
| GameplayFramework | ECS, ImGuiWrapper, Input, Renderer | \src{include/Gameplay/Components.hpp,10} |
