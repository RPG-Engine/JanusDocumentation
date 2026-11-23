# Janus Engine documentation examples
#
# Builds every example shown in the documentation against the engine
# (RPG-Engine-Core), which lives in its own repository. Each example is a single .cpp file with no external resources.
#
#   cmake -S examples -B <build dir> -G Ninja ^
#         -DCMAKE_TOOLCHAIN_FILE=<vcpkg>/scripts/buildsystems/vcpkg.cmake ^
#         -DVCPKG_MANIFEST_DIR=<Engine folder> ^
#         -DJANUS_ENGINE_DIR=<Engine folder>
#   cmake --build <build dir>
#
# Run an example from <build dir>/bin (the engine reads Resources/ from the
# current working directory). See "Using Janus > Building the examples".

cmake_minimum_required(VERSION 4.2)
project(JanusExamples CXX)

set(CMAKE_CXX_STANDARD 26)

# Put the engine's copied resources and every example executable in one folder.
# Setting OUTPUT_DIR_DETERMINED stops the engine's DetermineOutputDir.cmake from
# choosing its own output folder.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(OUTPUT_DIR_DETERMINED TRUE)

set(JANUS_ENGINE_DIR "" CACHE PATH "Path of the engine checkout (RPG-Engine-Core)")
if(NOT JANUS_ENGINE_DIR OR NOT EXISTS "${JANUS_ENGINE_DIR}/CMakeLists.txt")
    message(FATAL_ERROR "Set -DJANUS_ENGINE_DIR=<path to the engine checkout>")
endif()
add_subdirectory(${JANUS_ENGINE_DIR} Engine)

# The Filesystem module scans <working dir>/Resources/Config before anything
# else and throws if the folder is missing. It also reads the asset-type table
# from Resources/Config/filesystem.json. Put both in place next to the examples.
add_custom_target(JanusExampleResources ALL
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Resources/Config
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                ${JANUS_ENGINE_DIR}/Config/filesystem.json
                ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Resources/Config/filesystem.json
        COMMENT "Preparing Resources/Config for the examples")
add_dependencies(JanusExampleResources Engine)

function(janus_example NAME SOURCE)
    add_executable(${NAME} ${SOURCE})
    target_link_libraries(${NAME} PRIVATE Engine)
    add_dependencies(${NAME} JanusExampleResources)
endfunction()

# Game examples
janus_example(DodgeGame     games/Dodge.cpp)
janus_example(SnakeGame     games/Snake.cpp)
janus_example(BreakoutGame  games/Breakout.cpp)

# ECS examples
janus_example(EcsMovement   ecs/Movement.cpp)
janus_example(EcsSpawning   ecs/Spawning.cpp)
janus_example(EcsQueries    ecs/ComponentsAndQueries.cpp)
janus_example(EcsCollision  ecs/CollisionAndHover.cpp)

# Module examples
janus_example(HelloJanus      modules/HelloJanus.cpp)
janus_example(LoggingExample  modules/Logging.cpp)
janus_example(InputExample    modules/Input.cpp)
janus_example(AssetsExample   modules/AssetsAndMemory.cpp)
janus_example(RenderingExample modules/Rendering.cpp)
janus_example(EditorExample   modules/EditorTools.cpp)
