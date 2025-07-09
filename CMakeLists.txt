cmake_minimum_required(VERSION 3.31)
project(gce C)
set(CMAKE_C_STANDARD 99)
include(FetchContent)
cmake_policy(SET CMP0169 OLD)

# Download and build raylib
FetchContent_Declare(
        raylib
        GIT_REPOSITORY https://github.com/raysan5/raylib.git
        GIT_TAG 5.0
)

set(FETCHCONTENT_QUIET OFF)
set(BUILD_EXAMPLES OFF)
FetchContent_MakeAvailable(raylib)

# cJSON for perft
FetchContent_Declare(
        cJSON
        GIT_REPOSITORY https://github.com/DaveGamble/cJSON
        GIT_TAG v1.7.18
)

FetchContent_Populate(cjson)
add_library(cjson STATIC ${cjson_SOURCE_DIR}/cJSON.c)
target_include_directories(cjson PUBLIC ${cjson_SOURCE_DIR})

# Build GCE
include_directories(core/)

add_library(gce-core STATIC # Shared code
        core/core.h
        core/core.c
        core/moves.c
)

add_executable(gce-gui gui/ # GUI code
        gui/main.c
        gui/images.c
        gui/images.h
        gui/game.c
        gui/game.h
        gui/textures.h
        gui/textures.c)

target_link_libraries(gce-gui gce-core raylib)
if (APPLE) # raylib stuff for mac
    target_link_libraries(gce-gui "-framework IOKit")
    target_link_libraries(gce-gui "-framework Cocoa")
    target_link_libraries(gce-gui "-framework OpenGL")
endif ()

add_executable(gce-perft perft/ # perft testing
        perft/main.c)
target_link_libraries(gce-perft gce-core cjson)
