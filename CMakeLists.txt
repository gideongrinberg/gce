project(gce C CXX)
cmake_minimum_required(VERSION 3.25)
set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

set(CMAKE_C_FLAGS_RELEASE "-O3")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

option(BUILD_GUI "Build the GUI" ON)
option(BUILD_UCI "Build the UCI interface" ON)
option(BUILD_PERFT "Build perft test executable" ON)
option(BUILD_TESTS "Build unit tests" OFF)

option(ENABLE_PACKAGING "Enable packaging with CPack" OFF)
if (ENABLE_PACKAGING)
    include(InstallRequiredSystemLibraries)

    set(CPACK_PACKAGE_NAME "gce")
    set(CPACK_PACKAGE_VERSION "1.0.0")
    set(CPACK_GENERATOR "TGZ")

    include(CPack)
    set(INSTALL OFF CACHE BOOL "" FORCE) # prevent raylib from installing
endif ()

if (EMSCRIPTEN)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-unused-command-line-argument -s USE_GLFW=3 -s WASM=1 -s ASYNCIFY -s GL_ENABLE_GET_PROC_ADDRESS=1")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wno-unused-command-line-argument -s USE_GLFW=3 -s WASM=1 -s ASYNCIFY -s GL_ENABLE_GET_PROC_ADDRESS=1 -s EXPORTED_RUNTIME_METHODS=ccall,cwrap")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} --shell-file ${CMAKE_SOURCE_DIR}/shell.html")
    set(CMAKE_EXECUTABLE_SUFFIX ".html")
    set(PLATFORM "Web" CACHE STRING "Target platform" FORCE)

    set(BUILD_PERFT OFF CACHE BOOL "Disable perft for Web build" FORCE)
    set(BUILD_UCI OFF CACHE BOOL "Disable UCI for Web build" FORCE)
endif ()

# Download and build dependencies
if (BUILD_GUI)
    FetchContent_Declare(
            raylib
            GIT_REPOSITORY https://github.com/raysan5/raylib.git
            GIT_TAG 5.5
    )

    set(FETCHCONTENT_QUIET OFF)
    set(BUILD_EXAMPLES OFF)
    FetchContent_MakeAvailable(raylib)

    FetchContent_Populate(
            rlImgui
            GIT_REPOSITORY https://github.com/raylib-extras/rlImGui
            GIT_TAG main
            SOURCE_DIR ${CMAKE_BINARY_DIR}/rlImgui-src
    )

    FetchContent_Populate(
            imgui
            GIT_REPOSITORY https://github.com/ocornut/imgui
            GIT_TAG v1.92.1-docking
            SOURCE_DIR ${CMAKE_BINARY_DIR}/imgui-src
    )

    set(imgui_SOURCE_DIR ${CMAKE_BINARY_DIR}/imgui-src)
    set(rlImgui_SOURCE_DIR ${CMAKE_BINARY_DIR}/rlImgui-src)

    add_library(imgui STATIC
            ${imgui_SOURCE_DIR}/imgui.cpp
            ${imgui_SOURCE_DIR}/imgui_draw.cpp
            ${imgui_SOURCE_DIR}/imgui_demo.cpp
            ${imgui_SOURCE_DIR}/imgui_tables.cpp
            ${imgui_SOURCE_DIR}/imgui_widgets.cpp

            ${rlImgui_SOURCE_DIR}/rlImGui.cpp
    )

    target_link_libraries(imgui PUBLIC raylib)
    target_include_directories(imgui PUBLIC
            ${imgui_SOURCE_DIR}
            ${rlImgui_SOURCE_DIR}
            ${raylib_SOURCE_DIR}/src
    )

    FetchContent_Declare(json URL https://github.com/nlohmann/json/releases/download/v3.12.0/json.tar.xz)
    FetchContent_MakeAvailable(json)
endif ()

add_library(gce-core STATIC engine/position.c engine/position.h engine/tables.c engine/tables.h
        engine/engine.h engine/zobrist.h engine/zobrist.c
        engine/eval.c
        engine/eval.h
        engine/search.c
        engine/search.h)
target_include_directories(gce-core PUBLIC engine/)
if (ENABLE_PACKAGING)
    install(TARGETS gce-core
            ARCHIVE DESTINATION lib
            LIBRARY DESTINATION lib
            RUNTIME DESTINATION bin)
endif ()

if (NOT EMSCRIPTEN)
    if (BUILD_PERFT)
        add_executable(gce-perft perft/main.cpp)
        target_link_libraries(gce-perft gce-core)
        if (ENABLE_PACKAGING)
            install(TARGETS gce-perft RUNTIME DESTINATION bin)
        endif ()
    endif ()

    if (BUILD_UCI)
        add_executable(gce-uci uci/main.cpp uci/logger.hpp)
        target_link_libraries(gce-uci PUBLIC gce-core)
        if (ENABLE_PACKAGING)
            install(TARGETS gce-uci RUNTIME DESTINATION bin)
        endif ()
    endif ()

    if (BUILD_TESTS)
        add_executable(gce-test test/main.c test/tinytest.h)
        target_link_libraries(gce-test PUBLIC gce-core)
    endif ()
endif ()

if (BUILD_GUI)
    add_executable(gce-gui
            gui/main.cpp
            gui/board.hpp
            gui/board.cpp
            gui/assets.h
            gui/assets.c
            gui/textures.h
            gui/textures.c
            gui/window.hpp
            gui/game.hpp
            gui/info.hpp
            gui/game.cpp
            gui/modals.cpp
            gui/modals.hpp
            gui/polyglot.hpp
            gui/polyglot.cpp)

    target_link_libraries(gce-gui PRIVATE gce-core raylib imgui nlohmann_json::nlohmann_json)
    if (EMSCRIPTEN)
        set_target_properties(gce-gui PROPERTIES OUTPUT_NAME "index")
    endif ()
    if (ENABLE_PACKAGING)
        install(TARGETS gce-gui RUNTIME DESTINATION bin)
    endif ()

    if (APPLE) # raylib stuff for mac
        target_link_libraries(gce-gui PUBLIC "-framework IOKit")
        target_link_libraries(gce-gui PUBLIC "-framework Cocoa")
        target_link_libraries(gce-gui PUBLIC "-framework OpenGL")
    endif ()
endif ()