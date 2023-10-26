cmake_minimum_required(VERSION 3.12)

project(my_kernel)

# Set the compiler flags for SYCL
set(CMAKE_CXX_COMPILER "icpx")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsycl")

# Include directories for Intel oneAPI SDK
target_include_directories(my_kernel PRIVATE
    /opt/intel/oneapi/compiler/latest/linux/include
    /opt/intel/oneapi/compiler/latest/linux/include/sycl
)

# Your C++ source
add_library(my_kernel MODULE main.cpp)

# Link against SYCL library
target_link_libraries(my_kernel PRIVATE sycl)

# Specify C++ standard
target_compile_features(my_kernel PRIVATE cxx_std_17)

# Set the Python module extension to .so
set_target_properties(my_kernel PROPERTIES PREFIX ""
                                              SUFFIX ".so")
