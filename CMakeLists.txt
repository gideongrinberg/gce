cmake_minimum_required(VERSION 3.31)
project(gce C)
set(CMAKE_C_STANDARD 99)
include(FetchContent)
cmake_policy(SET CMP0169 OLD)
set(CMAKE_POLICY_VERSION_MINIMUM 3.5)

if (EMSCRIPTEN)
    #    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -s USE_GLFW=3 -s ASSERTIONS=1 -s WASM=1 -s ASYNCIFY -s GL_ENABLE_GET_PROC_ADDRESS=1")
    #    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -s USE_GLFW=3 -s FULL_ES3 -s ASSERTIONS=1 -s WASM=1 -s ASYNCIFY -s GL_ENABLE_GET_PROC_ADDRESS=1 --shell-file ${CMAKE_SOURCE_DIR}/shell.html")
    set(CMAKE_EXECUTABLE_SUFFIX ".html")
    set(PLATFORM "Web" CACHE STRING "Target platform" FORCE)
endif ()

set(CMAKE_C_FLAGS_DEBUG "-fsanitize=address,undefined -Wall -Wno-deprecated-declarations")
set(CMAKE_EXE_LINKER_FLAGS_DEBUG "-fsanitize=address,undefined -Wall")

# Download and build raylib
FetchContent_Declare(
        raylib
        GIT_REPOSITORY https://github.com/raysan5/raylib.git
        GIT_TAG 5.0
)

set(FETCHCONTENT_QUIET OFF)
set(BUILD_EXAMPLES OFF)
FetchContent_MakeAvailable(raylib)

# Add raylib nuklear
FetchContent_Declare(
        raylib_nuklear
        GIT_REPOSITORY https://github.com/RobLoach/raylib-nuklear
        GIT_TAG v5.5.1
)

FetchContent_MakeAvailable(raylib_nuklear)

# cJSON for perft
FetchContent_Declare(
        cJSON
        GIT_REPOSITORY https://github.com/DaveGamble/cJSON
        GIT_TAG v1.7.18
)

FetchContent_Populate(cjson)
if (!EMSCRIPTEN)
    add_library(cjson STATIC ${cjson_SOURCE_DIR}/cJSON.c)
    target_include_directories(cjson PUBLIC ${cjson_SOURCE_DIR})
endif ()

# Build GCE
include_directories(core/)

add_library(gce-core STATIC # Shared code
        core/core.h
        core/core.c
        core/moves.c
        core/search.c
        core/eval.c
)

# GUI code
add_executable(gce-gui gui/
        gui/main.c
        gui/assets.c
        gui/assets.h
        gui/game.c
        gui/game.h
        gui/textures.h
        gui/textures.c)

if (EMSCRIPTEN)
    target_link_options(gce-gui PRIVATE
            "-sUSE_GLFW=3"
            "-sFULL_ES2"
            "-sASSERTIONS=1"
            "-sWASM=1"
            "-sASYNCIFY"
            "-sGL_ENABLE_GET_PROC_ADDRESS=1"
            "--shell-file=${CMAKE_SOURCE_DIR}/shell.html"
    )
endif ()
# Generate headers to embed assets
add_custom_command(
        OUTPUT ${CMAKE_SOURCE_DIR}/gui/images.h ${CMAKE_SOURCE_DIR}/gui/images.c
        COMMAND python3 ${CMAKE_SOURCE_DIR}/gui/assets/build_assets.py
        DEPENDS ${CMAKE_SOURCE_DIR}/gui/assets/build_assets.py
)

add_custom_target(gui_assets
        DEPENDS ${CMAKE_SOURCE_DIR}/gui/images.h ${CMAKE_SOURCE_DIR}/gui/images.c
)

add_dependencies(gce-gui gui_assets)
target_link_libraries(gce-gui gce-core raylib raylib_nuklear)
if (APPLE) # raylib stuff for mac
    target_link_libraries(gce-gui "-framework IOKit")
    target_link_libraries(gce-gui "-framework Cocoa")
    target_link_libraries(gce-gui "-framework OpenGL")
endif ()

if (!EMSCRIPTEN)
    # perft testing
    add_executable(gce-perft perft/
            perft/main.c)
    target_link_libraries(gce-perft gce-core cjson)

    # UCI interface
    add_executable(gce-uci uci/ uci/main.c
            uci/main.c)
    target_link_libraries(gce-uci gce-core)
endif ()