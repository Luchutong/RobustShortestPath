cmake_minimum_required(VERSION 3.16)
project(RobustShortestPath LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(RSP_ENABLE_SANITIZERS "Enable Address/Undefined sanitizers" OFF)

add_compile_options(-Wall -Wextra -Wpedantic)

if (RSP_ENABLE_SANITIZERS)
    add_compile_options(-fsanitize=address,undefined -fno-omit-frame-pointer)
    add_link_options(-fsanitize=address,undefined)
endif()

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

add_executable(run_robustness experiments/run_robustness.cpp)
target_link_libraries(run_robustness PRIVATE rsp)

add_executable(run_runtime experiments/run_runtime.cpp)
target_link_libraries(run_runtime PRIVATE rsp)

add_executable(test_toy tests/test_toy.cpp)
target_link_libraries(test_toy PRIVATE rsp)

add_executable(test_hhm tests/test_hhm.cpp)
target_link_libraries(test_hhm PRIVATE rsp)

add_executable(test_lct tests/test_lct.cpp)
target_link_libraries(test_lct PRIVATE rsp)
target_compile_definitions(test_lct PRIVATE
    RSP_RUN_RUNTIME="$<TARGET_FILE:run_runtime>"
    RSP_RUN_ROBUSTNESS="$<TARGET_FILE:run_robustness>"
    RSP_MAIN_BIN="$<TARGET_FILE:rsp_main>"
)

enable_testing()
add_test(NAME test_toy COMMAND test_toy)
add_test(NAME test_hhm COMMAND test_hhm)
add_test(NAME test_lct COMMAND test_lct)
set_tests_properties(test_toy PROPERTIES WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
set_tests_properties(test_hhm PROPERTIES WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
set_tests_properties(test_lct PROPERTIES WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
