cmake_minimum_required(VERSION 3.10)

project(kiyanka)

add_executable(kiyanka WIN32 kiyanka.cc atto/src/app_windows.c)
target_include_directories(kiyanka PRIVATE atto/include atto/include/khronos)

# FIXME find_package(OpenGL) ...
target_link_libraries(kiyanka PRIVATE opengl32)
