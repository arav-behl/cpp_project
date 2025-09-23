cmake_minimum_required(VERSION 3.20)
project(realtime_cpp LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Compiler-specific optimizations
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    add_compile_options(-O3 -march=native -DNDEBUG -Wall -Wextra -Wpedantic)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    add_compile_options(/O2 /DNDEBUG /W4)
endif()

# Core library
add_library(rt_core
    src/md/feed_sim.cpp
    src/stats/rolling_stats.cpp
    src/stats/rolling_covar.cpp
    src/engine/signal_rules.cpp
    src/engine/router.cpp
    src/util/latency.cpp
)

target_include_directories(rt_core PUBLIC include)

# Find threads library for std::jthread
find_package(Threads REQUIRED)
target_link_libraries(rt_core PUBLIC Threads::Threads)

# Demo executable
add_executable(demo_realtime examples/demo_realtime.cpp)
target_link_libraries(demo_realtime PRIVATE rt_core)

# Test executable (optional)
add_executable(test_suite tests/test_stats.cpp tests/test_queue.cpp)
target_link_libraries(test_suite PRIVATE rt_core)

# Enable testing
enable_testing()
add_test(NAME CoreTests COMMAND test_suite)