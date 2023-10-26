cmake_minimum_required(VERSION 3.12)

project(my_kernel)

# Check if ONEAPI_ROOT is set (you can replace this with any variable that should be set by setvars.sh)
if(NOT DEFINED ENV{ONEAPI_ROOT})
    message(WARNING " 
    +-----------------------------------------------------------------------------+
    |                                                                             |
    |  WARNING: ONEAPI_ROOT is not set!                                           |
    |  Please ensure you `source /opt/intel/oneapi/setvars.sh` before running make. |
    |                                                                             |
    +-----------------------------------------------------------------------------+
    ")
endif()

# Set the compiler flags for SYCL
set(CMAKE_CXX_COMPILER "icpx")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsycl")

# Your C++ source
add_executable(my_kernel matmul.cpp)

# Include directories for Intel oneAPI SDK
target_include_directories(my_kernel PRIVATE
    /opt/intel/oneapi/compiler/latest/linux/include
    /opt/intel/oneapi/compiler/latest/linux/include/sycl
)

# Link against SYCL library
target_link_libraries(my_kernel PRIVATE sycl)

# Specify C++ standard
target_compile_features(my_kernel PRIVATE cxx_std_17)
