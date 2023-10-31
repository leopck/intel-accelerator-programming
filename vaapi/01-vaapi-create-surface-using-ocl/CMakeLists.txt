cmake_minimum_required(VERSION 3.11 FATAL_ERROR)
project(va_main)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find necessary packages
find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBAV REQUIRED IMPORTED_TARGET
    libva
    libva-drm
    libavdevice
    libavfilter
    libavformat
    libavcodec
    libswresample
    libswscale
    libavutil
)
set(OpenCL_INCLUDE_DIR "/usr/include/CL/")
set(OpenCL_LIBRARY "/opt/intel/oneapi/lib/libOpenCL.so")

find_package(OpenCL REQUIRED)

find_path(DRM_HEADERS drm_fourcc.h
          HINTS /usr/include/libdrm /usr/include/drm)
if(NOT DRM_HEADERS)
    message(FATAL_ERROR "drm_fourcc.h not found")
endif()

find_library(DRM_LIBRARIES drm)
if(NOT DRM_LIBRARIES)
    message(FATAL_ERROR "libdrm not found")
endif()

# Specify to build an executable, not a library
add_executable(va_main va_main.cpp)

# Add the include path and other include directories
target_include_directories(va_main PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/va_main
    ${DRM_HEADERS}
    ${OpenCL_INCLUDE_DIRS}
)

# Link against the LIBAV and other necessary libraries
target_link_libraries(va_main PRIVATE 
    PkgConfig::LIBAV
    ${DRM_LIBRARIES}
    ${OpenCL_LIBRARIES}
)
