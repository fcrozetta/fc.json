cmake_minimum_required(VERSION 3.21)
project(fc-json VERSION 2.0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(FC_JSON_BUILD_TESTS "Build the fc-json test suite" ON)

# Dependencies come from vcpkg (manifest mode). See ADR 0002.
find_package(RapidJSON CONFIG REQUIRED)  # header-only: sets RAPIDJSON_INCLUDE_DIRS
find_package(argparse CONFIG REQUIRED)   # target: argparse::argparse
find_package(tabulate CONFIG REQUIRED)   # target: tabulate::tabulate

# Core library: parse adapter + IR. rapidjson is confined to parser.cpp, so it
# is a PRIVATE include of the core and never leaks to consumers. See ADR 0001.
add_library(fc-json-core STATIC
  src/parse/parser.cpp
  src/schema/infer.cpp
  src/render/pydantic.cpp
  src/render/jsonschema.cpp
  src/render/document.cpp
  src/render/table.cpp
)
target_include_directories(fc-json-core PRIVATE ${RAPIDJSON_INCLUDE_DIRS})
target_link_libraries(fc-json-core PRIVATE tabulate::tabulate)

# The executable no longer touches rapidjson — it is fully behind the core
# library's parse adapter, so no rapidjson include here. See ADR 0001.
add_executable(fc-json src/main.cpp)
target_link_libraries(fc-json PRIVATE argparse::argparse fc-json-core)

# Version string baked into the binary. Defaults to the project version;
# release CI overrides it with the pushed tag (-DFC_VERSION=<tag>). See the
# release/version-define claim.
set(FC_VERSION "${PROJECT_VERSION}" CACHE STRING "fc-json version string")
target_compile_definitions(fc-json PRIVATE FC_VERSION="${FC_VERSION}")

if(FC_JSON_BUILD_TESTS)
  enable_testing()
  find_package(doctest CONFIG REQUIRED)
  add_executable(fc-json-tests
    src/tests/test_main.cpp
    src/tests/test_parser.cpp
    src/tests/test_infer.cpp
    src/tests/test_render.cpp
    src/tests/test_jsonschema.cpp
    src/tests/test_document.cpp
    src/tests/test_table.cpp
  )
  target_link_libraries(fc-json-tests PRIVATE doctest::doctest fc-json-core)
  add_test(NAME fc-json-tests COMMAND fc-json-tests)
endif()
