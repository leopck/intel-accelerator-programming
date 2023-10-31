cmake_minimum_required(VERSION 3.11 FATAL_ERROR)
project(va_main)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find necessary packages
find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBAV REQUIRED IMPORTED_TARGET
    libva libva-drm
    libze_loader
    libavdevice
    libavfilter
    libavformat
    libavcodec
    libswresample
    libswscale
    libavutil
)

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

# Add the dpcpp include path and other include directories
target_include_directories(va_main PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/va_main
    ${FFMPEG_INCLUDE_DIRS}
    ${DRM_HEADERS}
)

# Link against the LIBAV
target_link_libraries(va_main PRIVATE 
    PkgConfig::LIBAV
    ${DRM_LIBRARIES}
)
