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

# perft testing
add_custom_command(
        OUTPUT perft_tests.h
        COMMAND python3 ${CMAKE_SOURCE_DIR}/perft/generate_cases.py | xxd -n test_cases_json -i - > perft_tests.h
        DEPENDS ${CMAKE_SOURCE_DIR}/perft/generate_cases.py ${CMAKE_SOURCE_DIR}/perft/perftsuite.epd
        COMMENT "Embedding test cases"
)

add_custom_target(perft_test_cases DEPENDS ${CMAKE_BINARY_DIR}/perft_tests.h)
add_executable(gce-perft perft/
        perft/main.c ${CMAKE_BINARY_DIR}/perft_tests.h)
add_dependencies(gce-perft perft_test_cases)
target_link_libraries(gce-perft gce-core cjson)
target_include_directories(gce-perft PRIVATE ${CMAKE_BINARY_DIR})