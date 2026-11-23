# How to read these docs {#conventions}

<p class="lede">How the pages are laid out, how code examples are marked, and how source links work.</p>

## Page types

- **Guides** (\ref using_janus, \ref engine_organization) explain concepts. Read them top to bottom.
- **Module pages** (like \ref ecs) start with what the module does and **How to use** it, then explain the details and pitfalls, then list every class, function and macro in the module.
- **Class pages** (like Log::SLogger) start with a summary and how to use the class, then list its members.
- **Example pages** (\ref examples) show a screenshot, walk through the important code, and end with the full source.

Only the **public** API that game code needs is documented. Private members and internal helpers are left out.

Use **On this page** (right) to jump within a page, and the search box in the top bar (<kbd>Ctrl</kbd> <kbd>K</kbd>) to find any class, function or macro.

## Code examples

Code blocks come in two kinds, marked by their header. Click a header to open or close the block.

### Engine examples

Green blocks contain real code from the engine. The header names the file and line, linked to GitHub. Snippets may be trimmed with `// ...`, but the code that remains is unchanged.

\engexopen{src/Filesystem/Module.cpp,10,10,The Filesystem module declares its logger}
```cpp
DECLARE_LOGGER(Filesystem, debug)
```
\endex

### AI-generated examples

Example blocks without a label contain code written for this documentation by AI, not taken from the engine. Pages that contain them start with an <b>AI-generated content</b> notice. Each block comes from a program in the `examples` folder of the JanusDocumentation repository, named and linked in the block's header. The documentation build pulls the code straight from those files, so what you read is what was compiled. All of them were compiled and run against the documented commit (\ref docs_version), but they are not part of the engine and have not been reviewed by its authors.

\genexopen{modules/HelloJanus.cpp,A generated example}
\snippet modules/HelloJanus.cpp solid_texture
\endex

The example files contain comment markers such as `//! [solid_texture]`. They name the sections the pages show; leave them in place when you edit an example.

### Other code

Short fragments without a header (one or two lines in a guide, command lines, CMake files, file layouts) are written for these pages. The CMake project in \ref first_game was built and run like the examples.

## Source links

Links marked ↗ open the code on GitHub, at the exact line or line range where possible. They all point at one pinned commit, \docscommit, so line numbers always match the code they describe. \ref docs_version explains how the pin is moved when the docs are updated.

## Naming prefixes used by the engine

| Prefix | Meaning | Example |
| --- | --- | --- |
| `S` | Struct: plain data or a value type | `SLogger`, `SAssetData` |
| `F` | ECS component | `FTransformComponent` |
| `E` | Enum | `EAssetType`, `EInstanceLayer` |
| `C` | C++20 concept | `CIs_Asset` |
| `T` | Template type | `TObjectPtr`, `STypedSoftAssetPath<T>` |
| `I` | Interface | `IEditorTool` |
| `b` | Boolean variable | `bShouldDisplayDebugColliders`, `bOpen` |
