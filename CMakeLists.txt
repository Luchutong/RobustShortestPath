cmake_minimum_required(VERSION 3.16)
project(RobustShortestPath LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(rsp
    src/graph.cpp
    src/bellman.cpp
    src/value_iteration.cpp
    src/proper_policy.cpp
    src/policy_iteration.cpp
    src/dijkstra_like.cpp
    src/exhaustive_search.cpp
    src/baseline.cpp
    src/runner.cpp
    src/io.cpp
)

target_include_directories(rsp PUBLIC include)

add_executable(rsp_main src/main.cpp)
target_link_libraries(rsp_main PRIVATE rsp)

add_executable(test_toy tests/test_toy.cpp)
target_link_libraries(test_toy PRIVATE rsp)

enable_testing()
add_test(NAME test_toy COMMAND test_toy)
set_tests_properties(test_toy PROPERTIES WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
