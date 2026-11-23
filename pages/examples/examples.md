# Examples {#examples}

<p class="lede">Complete programs that use only Janus. Each is a single <code>.cpp</code> file, needs no image or data files (textures are generated in code), and was compiled and run against the documented engine commit.</p>

Build them all at once from the `examples` folder of the JanusDocumentation repository; see \ref getting_started_examples. Each page shows a screenshot, explains the important parts, and ends with the full source.

The examples start with `using namespace Components;`, so the engine's components are written without their namespace: `FTransformComponent` is `Components::FTransformComponent`. Lines are kept to 88 characters so the code fits the page without wrapping.

## Game examples {#examples_games}

Three small games that show how the pieces fit together. They are meant to be read, not to be good games.

<div class="jd-cards">
<a class="jd-card" href="ex_dodge.html"><span class="jd-card-title">Dodge</span><span class="jd-card-text">Avoid falling blocks. Input, spawning on a timer, destroying entities, collision with a query, a HUD.</span></a>
<a class="jd-card" href="ex_snake.html"><span class="jd-card-title">Snake</span><span class="jd-card-text">Grid movement on a fixed time step, game data separate from presentation, reusing entities.</span></a>
<a class="jd-card" href="ex_breakout.html"><span class="jd-card-title">Breakout</span><span class="jd-card-text">A bouncing ball, paddle angles, breaking bricks, lives and a win condition.</span></a>
</div>

## ECS examples {#examples_ecs}

Focused examples of common mechanics built with entities, components and systems.

<div class="jd-cards">
<a class="jd-card" href="ex_movement.html"><span class="jd-card-title">Moving entities</span><span class="jd-card-text">Input to velocity to position, bouncing, and a camera that follows the player.</span></a>
<a class="jd-card" href="ex_spawning.html"><span class="jd-card-title">Spawning and destroying</span><span class="jd-card-text">Spawners, prefabs, lifetimes and fading out.</span></a>
<a class="jd-card" href="ex_queries.html"><span class="jd-card-title">Components, queries and observers</span><span class="jd-card-text">The component macros, tags, queries, observers, a custom phase and global settings.</span></a>
<a class="jd-card" href="ex_collision.html"><span class="jd-card-title">Collision and mouse hover</span><span class="jd-card-text">Colliders, sliding along walls, the collider overlay and hovering with the mouse.</span></a>
</div>

## Module examples {#examples_modules}

One example per engine module.

<div class="jd-cards">
<a class="jd-card" href="ex_hello.html"><span class="jd-card-title">Hello Janus</span><span class="jd-card-text">The smallest program that shows something. Engine core.</span></a>
<a class="jd-card" href="ex_logging.html"><span class="jd-card-title">Logging</span><span class="jd-card-text">Loggers, levels, outputs and log files. A console program.</span></a>
<a class="jd-card" href="ex_input.html"><span class="jd-card-title">Input</span><span class="jd-card-text">Press, hold and release bindings, combos, operators, callbacks, quitting.</span></a>
<a class="jd-card" href="ex_assets.html"><span class="jd-card-title">Assets and memory</span><span class="jd-card-text">Asset paths, loading and caching, custom asset types, generated textures, TObjectPtr.</span></a>
<a class="jd-card" href="ex_rendering.html"><span class="jd-card-title">Rendering</span><span class="jd-card-text">Layers, sprite-sheet cells, tint, rotation, Y-sorting and coloured lights.</span></a>
<a class="jd-card" href="ex_editor.html"><span class="jd-card-title">Editor tools</span><span class="jd-card-text">Your own ImGui tool windows, opening and closing tools, the built-in tools.</span></a>
</div>

## Which example covers what

| Module | Examples |
| --- | --- |
| \ref engine_core | All of them; \ref ex_hello is the shortest |
| \ref logging | \ref ex_logging; every example declares a logger |
| \ref filesystem | \ref ex_assets; every example registers generated textures |
| \ref memory | \ref ex_assets |
| \ref ecs | \ref ex_movement, \ref ex_spawning, \ref ex_queries, \ref ex_collision, all games |
| \ref rendering | \ref ex_rendering, \ref ex_hello, all games |
| \ref input | \ref ex_input, \ref ex_movement, all games |
| \ref coretypes | \ref ex_assets (textures), \ref ex_editor (frame timing), \ref ex_collision (debug colliders) |
| \ref gameplay | \ref ex_collision |
| \ref imguiwrapper | \ref ex_editor, every HUD |
| \ref editor | \ref ex_editor, every HUD |

## The parts every example shares

Every graphical example starts with the same few helpers, because the renderer needs them (see \ref essentials_minimum):

- `RegisterSolidTexture("CollisionTexture", 8)` and `RegisterSolidTexture("White", 8)` before Engine::InitializePlugins(): textures made in code, so no image files are needed. Sprites tint the white texture with their colour.
- `CreateCameraAndBackground(...)`: a fixed camera showing 320 × 180 world units, a light that covers the screen, and a full-screen sprite on the Background layer.
- A small HUD tool that collapses the Dear ImGui demo window and shows the controls.
- `std::quick_exit(0)` at the end of `main()`, to avoid a crash after shutdown (\ref known_issues_shutdown).

\htmlonly<div style="display:none">\endhtmlonly
\subpage ex_dodge
\subpage ex_snake
\subpage ex_breakout
\subpage ex_movement
\subpage ex_spawning
\subpage ex_queries
\subpage ex_collision
\subpage ex_hello
\subpage ex_logging
\subpage ex_input
\subpage ex_assets
\subpage ex_rendering
\subpage ex_editor
\htmlonly</div>\endhtmlonly
