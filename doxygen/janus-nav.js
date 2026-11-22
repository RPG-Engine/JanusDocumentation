/*
 * Sidebar navigation for the Janus Engine docs.
 *
 * Hand-curated so the sidebar reads like a manual rather than a raw symbol tree.
 * When you add a page or document a class, add it here too. All hrefs are Doxygen
 * output file names (every page lives in the same folder). Doxygen derives them
 * from page ids and symbol names:
 *   \page ex_dodge            -> ex_dodge.html
 *   group "engine_core"       -> group__engine__core.html
 *   struct Log::SLogger       -> struct_log_1_1_s_logger.html
 */
window.JANUS_NAV = [
  {
    title: 'Getting started',
    items: [
      { label: 'Introduction', href: 'index.html' },
      {
        label: 'Using Janus', href: 'using_janus.html',
        children: [
          { label: 'Installing and building', href: 'getting_started.html' },
          { label: 'Your first game', href: 'first_game.html' },
          { label: 'Things to know', href: 'essentials.html' }
        ]
      },
      { label: 'How the engine is organized', href: 'engine_organization.html' }
    ]
  },
  {
    title: 'Engine modules',
    items: [
      { label: 'Overview', href: 'modules_overview.html' },
      { label: 'Engine core', href: 'group__engine__core.html' },
      {
        label: 'ECS', href: 'group__ecs.html',
        children: [
          { label: 'FTransformComponent', href: 'struct_components_1_1_f_transform_component.html', code: true },
          { label: 'FCameraComponent', href: 'struct_components_1_1_f_camera_component.html', code: true },
          { label: 'FLightComponent', href: 'struct_components_1_1_f_light_component.html', code: true },
          { label: 'FBoundingBoxComponent', href: 'struct_components_1_1_f_bounding_box_component.html', code: true }
        ]
      },
      {
        label: 'Renderer', href: 'group__rendering.html',
        children: [
          { label: 'FTexture2DComponent', href: 'struct_components_1_1_f_texture2_d_component.html', code: true },
          { label: 'FRenderingComponent', href: 'struct_components_1_1_f_rendering_component.html', code: true },
          { label: 'RenderLayer', href: 'struct_renderer_1_1_render_layer.html', code: true }
        ]
      },
      {
        label: 'Input', href: 'group__input.html',
        children: [
          { label: 'SAction', href: 'struct_input_1_1_s_action.html', code: true },
          { label: 'SInputBinding', href: 'struct_input_1_1_s_input_binding.html', code: true }
        ]
      },
      {
        label: 'Filesystem', href: 'group__filesystem.html',
        children: [
          { label: 'SAssetData', href: 'struct_filesystem_1_1_s_asset_data.html', code: true },
          { label: 'SSoftAssetPath', href: 'struct_filesystem_1_1_s_soft_asset_path.html', code: true },
          { label: 'SAsset', href: 'struct_s_asset.html', code: true }
        ]
      },
      {
        label: 'CoreEditor', href: 'group__editor.html',
        children: [
          { label: 'IEditorTool', href: 'struct_im_g_u_i_wrapper_1_1_i_editor_tool.html', code: true }
        ]
      },
      { label: 'ImGuiWrapper', href: 'group__imguiwrapper.html' },
      {
        label: 'GameplayFramework', href: 'group__gameplay.html',
        children: [
          { label: 'FColliderComponent', href: 'struct_components_1_1_f_collider_component.html', code: true },
          { label: 'FMouseState', href: 'struct_components_1_1_f_mouse_state.html', code: true }
        ]
      },
      {
        label: 'CoreTypes', href: 'group__coretypes.html',
        children: [
          { label: 'STexture2D', href: 'struct_graphics_1_1_s_texture2_d.html', code: true },
          { label: 'SSprite', href: 'struct_graphics_1_1_s_sprite.html', code: true },
          { label: 'SLevel', href: 'struct_world_1_1_s_level.html', code: true }
        ]
      },
      {
        label: 'Memory', href: 'group__memory.html',
        children: [
          { label: 'TObjectPtr', href: 'class_t_object_ptr.html', code: true }
        ]
      },
      {
        label: 'Log', href: 'group__logging.html',
        children: [
          { label: 'SLogger', href: 'struct_log_1_1_s_logger.html', code: true },
          { label: 'SLoggerConfiguration', href: 'struct_log_1_1_s_logger_configuration.html', code: true }
        ]
      }
    ]
  },
  {
    title: 'Examples',
    items: [
      { label: 'All examples', href: 'examples.html' },
      {
        label: 'Game examples', href: 'examples.html#examples_games',
        children: [
          { label: 'Dodge', href: 'ex_dodge.html' },
          { label: 'Snake', href: 'ex_snake.html' },
          { label: 'Breakout', href: 'ex_breakout.html' }
        ]
      },
      {
        label: 'ECS', href: 'examples.html#examples_ecs',
        children: [
          { label: 'Moving entities', href: 'ex_movement.html' },
          { label: 'Spawning and destroying', href: 'ex_spawning.html' },
          { label: 'Components, queries, observers', href: 'ex_queries.html' },
          { label: 'Collision and mouse hover', href: 'ex_collision.html' }
        ]
      },
      {
        label: 'Modules', href: 'examples.html#examples_modules',
        children: [
          { label: 'Hello Janus', href: 'ex_hello.html' },
          { label: 'Logging', href: 'ex_logging.html' },
          { label: 'Input', href: 'ex_input.html' },
          { label: 'Assets and memory', href: 'ex_assets.html' },
          { label: 'Rendering', href: 'ex_rendering.html' },
          { label: 'Editor tools', href: 'ex_editor.html' }
        ]
      }
    ]
  },
  {
    title: 'About these docs',
    items: [
      { label: 'Known issues', href: 'known_issues.html' },
      { label: 'Documentation version', href: 'docs_version.html' },
      { label: 'How to read these docs', href: 'conventions.html' }
    ]
  },
  {
    title: 'Reference',
    items: [
      { label: 'Class list', href: 'annotated.html' },
      { label: 'Class index', href: 'classes.html' },
      { label: 'Namespaces', href: 'namespaces.html' },
      { label: 'Macros and functions', href: 'globals.html' },
      { label: 'All modules', href: 'topics.html' }
    ]
  }
];
