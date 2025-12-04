cmake_minimum_required(VERSION 3.16)
project(BreakingtheSorting VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -Wall -Wextra")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")

# Options
option(BUILD_TESTS "Build unit tests" ON)
option(BUILD_BENCHMARKS "Build benchmarks" ON)

# Find LEMON library
find_package(LEMON QUIET)
if(NOT LEMON_FOUND)
    # Try to find LEMON manually
    find_path(LEMON_INCLUDE_DIR lemon/list_graph.h
        PATHS /usr/local/include /usr/include /opt/homebrew/include
    )
    find_library(LEMON_LIBRARY NAMES lemon emon
        PATHS /usr/local/lib /usr/lib /opt/homebrew/lib
    )
    if(LEMON_INCLUDE_DIR AND LEMON_LIBRARY)
        set(LEMON_FOUND TRUE)
        set(LEMON_INCLUDE_DIRS ${LEMON_INCLUDE_DIR})
        set(LEMON_LIBRARIES ${LEMON_LIBRARY})
        message(STATUS "Found LEMON: ${LEMON_LIBRARY}")
    else()
        message(STATUS "LEMON not found - will fetch and build from source")
        include(FetchContent)
        FetchContent_Declare(
            lemon
            URL https://lemon.cs.elte.hu/pub/sources/lemon-1.3.1.tar.gz
            URL_HASH MD5=e89f73b7c23f9c4154a7760f6f3e4c10
        )
        FetchContent_MakeAvailable(lemon)
        set(LEMON_INCLUDE_DIRS ${lemon_SOURCE_DIR} ${lemon_BINARY_DIR})
        set(LEMON_LIBRARIES lemon)
    endif()
endif()

# Fetch Google Test
include(FetchContent)
if(BUILD_TESTS)
    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG v1.14.0
    )
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)
    enable_testing()
endif()

# Fetch Google Benchmark
if(BUILD_BENCHMARKS)
    set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "" FORCE)
    set(BENCHMARK_ENABLE_GTEST_TESTS OFF CACHE BOOL "" FORCE)
    FetchContent_Declare(
        benchmark
        GIT_REPOSITORY https://github.com/google/benchmark.git
        GIT_TAG v1.8.3
    )
    FetchContent_MakeAvailable(benchmark)
endif()

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${LEMON_INCLUDE_DIRS})

# Main library
add_library(sssp_lib INTERFACE)
target_include_directories(sssp_lib INTERFACE ${CMAKE_SOURCE_DIR}/include)

# Tests
if(BUILD_TESTS)
    add_executable(sssp_tests
        tests/test_main.cpp
        tests/test_graph.cpp
        tests/test_dijkstra.cpp
        tests/test_new_sssp.cpp
        tests/test_correctness.cpp
    )
    target_link_libraries(sssp_tests
        sssp_lib
        ${LEMON_LIBRARIES}
        GTest::gtest
        GTest::gtest_main
    )
    target_include_directories(sssp_tests PRIVATE ${LEMON_INCLUDE_DIRS})

    include(GoogleTest)
    gtest_discover_tests(sssp_tests)
endif()

# Benchmarks
if(BUILD_BENCHMARKS)
    add_executable(sssp_benchmarks
        benchmarks/benchmark_main.cpp
    )
    target_link_libraries(sssp_benchmarks
        sssp_lib
        ${LEMON_LIBRARIES}
        benchmark::benchmark
        benchmark::benchmark_main
    )
    target_include_directories(sssp_benchmarks PRIVATE ${LEMON_INCLUDE_DIRS})
endif()

# Demo executable
add_executable(sssp_demo
    src/main.cpp
)
target_link_libraries(sssp_demo
    sssp_lib
    ${LEMON_LIBRARIES}
)
target_include_directories(sssp_demo PRIVATE ${LEMON_INCLUDE_DIRS})

# MTX Benchmark executable (main CLI tool)
add_executable(sssp_benchmark
    src/sssp_benchmark.cpp
)
target_link_libraries(sssp_benchmark
    sssp_lib
    ${LEMON_LIBRARIES}
)
target_include_directories(sssp_benchmark PRIVATE ${LEMON_INCLUDE_DIRS})
