# Janus %Engine documentation {#mainpage}

<p class="lede">Janus is a C++26 engine for 2D games. Games are built from entities and components with <a href="https://www.flecs.dev/flecs/">flecs</a>, drawn by a batched sprite renderer on SDL3's GPU API, and inspected live with Dear ImGui editor tools. It compiles into a single static library that your game links.</p>

<div class="jd-meta"><span><b>Documents engine commit</b> \docscommit</span><span><b>Updated</b> \docsdate</span><span>\ref docs_version "How this is kept up to date"</span></div>

## Start here

<div class="jd-cards">
<a class="jd-card" href="getting_started.html"><span class="jd-card-title">Install and build</span><span class="jd-card-text">Requirements, vcpkg, building the engine and the examples.</span></a>
<a class="jd-card" href="first_game.html"><span class="jd-card-title">Your first game</span><span class="jd-card-text">Set up a game project and put a sprite on screen, step by step.</span></a>
<a class="jd-card" href="essentials.html"><span class="jd-card-title">Things to know</span><span class="jd-card-text">The rules every Janus game follows: camera, lights, layers, assets, input.</span></a>
<a class="jd-card" href="examples.html"><span class="jd-card-title">Examples</span><span class="jd-card-text">Three small games and focused examples for the ECS and every module.</span></a>
</div>

## The manual

<div class="jd-cards">
<a class="jd-card" href="engine_organization.html"><span class="jd-card-title">How the engine is organized</span><span class="jd-card-text">Startup, the game loop, shutdown and how modules depend on each other.</span></a>
<a class="jd-card" href="modules_overview.html"><span class="jd-card-title">Engine modules</span><span class="jd-card-text">One page per module, each starting with how to use it.</span></a>
<a class="jd-card" href="known_issues.html"><span class="jd-card-title">Known issues</span><span class="jd-card-text">Engine behaviour that may surprise you, with workarounds.</span></a>
<a class="jd-card" href="annotated.html"><span class="jd-card-title">API reference</span><span class="jd-card-text">Every class, function and macro. Or press <kbd>Ctrl</kbd> <kbd>K</kbd> to search.</span></a>
</div>

## What a Janus game looks like

A game is an ordinary `main()` that starts the engine, creates entities, adds systems and runs the loop. This is the whole of the \ref ex_hello example, minus the texture helper:

\genexopen{modules/HelloJanus.cpp,Hello Janus: main()}
\snippet modules/HelloJanus.cpp main
\endex

<div class="jd-meta"><span>From \exsrc{modules/HelloJanus.cpp}, which is compiled and run against the documented commit.</span></div>

## What the engine gives you

| Area | What you get | Module |
| --- | --- | --- |
| Game objects | flecs entities, components and systems; component macros with reflection and editor panels | \ref ecs "ECS" |
| Drawing | Batched sprites in five layers, sprite-sheet cells, tint, rotation, Y-sorting, a camera, 2D lights | \ref rendering "Renderer" |
| Input | Named actions with press, hold and release bindings and value operators | \ref input "Input" |
| Assets | A registry of every file under `Resources/`, typed loaders, LDtk level import | \ref filesystem "Filesystem" |
| Tools | Dear ImGui with docking, an Entity Debugger, an FPS counter, your own tool windows | \ref editor "CoreEditor", \ref imguiwrapper "ImGuiWrapper" |
| Gameplay helpers | Colliders, mouse hover, a 2D movement action | \ref gameplay "GameplayFramework" |
| Basics | Logging, a reference-counted pointer, frame timing | \ref logging "Log", \ref memory "Memory", \ref coretypes "CoreTypes" |

## How these pages are organized

- **Using Janus** gets you from a fresh checkout to a running game, and lists what every game needs.
- **Engine modules** has a page per module. Each one opens with *How to use*, then explains the details, then lists the module's classes and functions.
- **Examples** are complete programs. Every one is a single `.cpp` file that uses no image or data files, and every one has been compiled and run.
- Code taken from the engine is marked **Engine example**. Code written for these docs by AI has no label, and its page starts with an AI-generated notice. See \ref conventions.
- Links marked ↗ open the source on GitHub at the exact line, at the commit these docs describe.

\htmlonly<div style="display:none">\endhtmlonly
\subpage using_janus
\subpage engine_organization
\subpage modules_overview
\subpage examples
\subpage known_issues
\subpage docs_version
\subpage conventions
\htmlonly</div>\endhtmlonly
