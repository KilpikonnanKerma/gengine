cmake_minimum_required(VERSION 3.15)
project(GENGINE)

set(CMAKE_CXX_STANDARD 17)

# Libs
find_package(SDL3 REQUIRED)
find_package(OpenGL REQUIRED)

include_directories(include source shaders)

file(GLOB ENGINE_SOURCES "Engine/*.cpp" "Engine/*.c" "Engine/Util/*.cpp" "source/*.cpp")
add_executable(GENGINE ${ENGINE_SOURCES})

file(COPY shaders DESTINATION ${CMAKE_BINARY_DIR})

target_include_directories(GENGINE PRIVATE include source shaders)
target_link_libraries(GENGINE PRIVATE SDL3::SDL3 OpenGL::GL)