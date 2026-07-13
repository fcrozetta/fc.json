cmake_minimum_required(VERSION 3.21)
project(fc-json LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Dependencies come from vcpkg (manifest mode). See ADR 0002.
find_package(RapidJSON CONFIG REQUIRED)  # header-only: sets RAPIDJSON_INCLUDE_DIRS
find_package(argparse CONFIG REQUIRED)   # target: argparse::argparse

add_executable(fc-json fc.json/main.cpp)
target_include_directories(fc-json PRIVATE ${RAPIDJSON_INCLUDE_DIRS})
target_link_libraries(fc-json PRIVATE argparse::argparse)
