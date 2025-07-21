cmake_minimum_required(VERSION 3.31)
project(gce C CXX)
cmake_policy(SET CMP0169 OLD)
set(CMAKE_POLICY_VERSION_MINIMUM 3.5)
set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 20)

include(FetchContent)

set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS} -O3")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS} -O3")

if (EMSCRIPTEN)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-unused-command-line-argument -s USE_GLFW=3 -s WASM=1 -s ASYNCIFY -s GL_ENABLE_GET_PROC_ADDRESS=1")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wno-unused-command-line-argument -s USE_GLFW=3 -s WASM=1 -s ASYNCIFY -s GL_ENABLE_GET_PROC_ADDRESS=1 -s EXPORTED_RUNTIME_METHODS=ccall,cwrap")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} --shell-file ${CMAKE_SOURCE_DIR}/shell.html")
    set(CMAKE_EXECUTABLE_SUFFIX ".html")
    set(PLATFORM "Web" CACHE STRING "Target platform" FORCE)
endif ()

# Download and build dependencies
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

add_library(gce-core STATIC engine/position.c engine/position.h engine/tables.c engine/tables.h
        engine/engine.h engine/zobrist.h engine/zobrist.c
        engine/eval.c
        engine/eval.h
        engine/search.c
        engine/search.h)
target_include_directories(gce-core PUBLIC engine/)

if (NOT EMSCRIPTEN)
    add_executable(gce-perft perft/main.cpp)
    target_link_libraries(gce-perft gce-core)

    add_executable(gce-uci uci/main.cpp uci/logger.hpp)
    target_link_libraries(gce-uci PUBLIC gce-core)

    add_executable(gce-test test/main.c test/tinytest.h)
    target_link_libraries(gce-test PUBLIC gce-core)
endif ()

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
        gui/modals.hpp)

target_link_libraries(gce-gui PRIVATE gce-core raylib imgui)

if (APPLE) # raylib stuff for mac
    target_link_libraries(gce-gui PUBLIC "-framework IOKit")
    target_link_libraries(gce-gui PUBLIC "-framework Cocoa")
    target_link_libraries(gce-gui PUBLIC "-framework OpenGL")
endif ()

if (EMSCRIPTEN)
    #    target_link_options(gce-gui PRIVATE
    #            "-sUSE_GLFW=3"
    #            "-sGL_DEBUG=1"
    #            "-sGL_ASSERTIONS=1"
    #            "-sALLOW_MEMORY_GROWTH=1"
    #            "-sFULL_ES3"
    #            "-sASSERTIONS=1"
    #            "-sWASM=1"
    #            "-sASYNCIFY"
    #            "-sGL_ENABLE_GET_PROC_ADDRESS=1"
    #            "-sEXPORTED_FUNCTIONS=_main,_on_browser_resize"
    #            "-sEXPORTED_RUNTIME_METHODS=ccall,cwrap"
    #            "--shell-file=${CMAKE_SOURCE_DIR}/shell.html"
    #    )
endif ()
