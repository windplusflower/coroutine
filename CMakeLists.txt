cmake_minimum_required(VERSION 3.10)  

add_definitions(-DUSE_DEBUG)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_COMPILER clang)

project(coroutine)  

add_subdirectory(src)
add_subdirectory(test)
