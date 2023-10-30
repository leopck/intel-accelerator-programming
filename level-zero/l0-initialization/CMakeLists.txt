cmake_minimum_required(VERSION 3.11 FATAL_ERROR)
project(ze_main)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Specify to build an executable, not a library
add_executable(ze_main ze_main.cpp)

# Add the dpcpp include path and other include directories
target_include_directories(ze_main PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/ze_main
    /usr/include/level_zero
)
