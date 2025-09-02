cmake_minimum_required(VERSION 3.15)

project(GENGINE)

set(CMAKE_MAKE_PROGRAM "C:/mingw64/bin/mingw32-make.exe")

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Libs
find_package(SDL2 REQUIRED)
find_package(OpenGL REQUIRED)

include_directories(include include/nsmlib/ source shaders)

file(GLOB ENGINE_SOURCES "Engine/*.cpp" "Engine/util/*.cpp" "source/*.cpp" "include/glad/*.c" )

file(GLOB IMGUI_SRC
    "include/imgui/*.cpp"
    "include/imgui/backends/imgui_impl_sdl2.cpp"
    "include/imgui/backends/imgui_impl_opengl3.cpp"
)

add_executable(GENGINE ${ENGINE_SOURCES} ${IMGUI_SRC})

add_definitions(-DIMGUI_IMPL_OPENGL_LOADER_GLAD)
add_definitions(-DIMGUI_ENABLE_DOCKING)

file(COPY shaders DESTINATION ${CMAKE_BINARY_DIR})
file(COPY textures DESTINATION ${CMAKE_BINARY_DIR})

target_compile_definitions(GENGINE PRIVATE GLM_ENABLE_EXPERIMENTAL)

target_include_directories(GENGINE PRIVATE include source shaders include/imgui include/nsmlib include/imgui/backends)
target_link_libraries(GENGINE PRIVATE SDL2::SDL2 OpenGL::GL)

# Lightweight player runtime (no editor/imgui)
file(GLOB PLAYER_SOURCES "Engine/*.cpp" "Engine/util/*.cpp" "source/*.cpp" "include/glad/*.c" )
list(FILTER PLAYER_SOURCES EXCLUDE REGEX "Editor\\.cpp$")
list(FILTER PLAYER_SOURCES EXCLUDE REGEX "imgui_.*\\.cpp$")
add_executable(gengine-player ${PLAYER_SOURCES} ${ENGINE_SOURCES} ${IMGUI_SRC})
target_compile_definitions(gengine-player PRIVATE GLM_ENABLE_EXPERIMENTAL)
target_include_directories(gengine-player PRIVATE include source shaders include/imgui include/nsmlib include/imgui/backends)
target_link_libraries(gengine-player PRIVATE SDL2::SDL2 OpenGL::GL)