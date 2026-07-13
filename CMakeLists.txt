cmake_minimum_required(VERSION 3.21)
project(fc-json LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Dependencies come from vcpkg (manifest mode). See ADR 0002.
find_package(RapidJSON CONFIG REQUIRED)  # header-only: sets RAPIDJSON_INCLUDE_DIRS
find_package(argparse CONFIG REQUIRED)   # target: argparse::argparse

# Core library: parse adapter + IR. rapidjson is confined to parser.cpp, so it
# is a PRIVATE include of the core and never leaks to consumers. See ADR 0001.
add_library(fc-json-core STATIC
  fc.json/parse/parser.cpp
)
target_include_directories(fc-json-core PRIVATE ${RAPIDJSON_INCLUDE_DIRS})

add_executable(fc-json fc.json/main.cpp)
target_include_directories(fc-json PRIVATE ${RAPIDJSON_INCLUDE_DIRS})
target_link_libraries(fc-json PRIVATE argparse::argparse fc-json-core)

# Tests
enable_testing()
find_package(doctest CONFIG REQUIRED)
add_executable(fc-json-tests fc.json/tests/test_parser.cpp)
target_link_libraries(fc-json-tests PRIVATE doctest::doctest fc-json-core)
add_test(NAME fc-json-tests COMMAND fc-json-tests)
