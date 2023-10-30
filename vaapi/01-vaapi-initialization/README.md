Sample execution:

```
root@8691191c6ae4:/app/codechest/vaapi/vaapi-initialization# cd build
root@8691191c6ae4:/app/codechest/vaapi/vaapi-initialization/build# cmake ..
-- The C compiler identification is GNU 11.4.0
-- The CXX compiler identification is GNU 11.4.0
-- Detecting C compiler ABI info
-- Detecting C compiler ABI info - done
-- Check for working C compiler: /usr/bin/cc - skipped
-- Detecting C compile features
-- Detecting C compile features - done
-- Detecting CXX compiler ABI info
-- Detecting CXX compiler ABI info - done
-- Check for working CXX compiler: /usr/bin/c++ - skipped
-- Detecting CXX compile features
-- Detecting CXX compile features - done
-- Found PkgConfig: /usr/bin/pkg-config (found version "0.29.2") 
-- Checking for modules 'libva;libva-drm;libze_loader;libavdevice;libavfilter;libavformat;libavcodec;libswresample;libswscale;libavutil'
--   Found libva, version 1.19.0
--   Found libva-drm, version 1.19.0
--   Found libze_loader, version 1.11.0
--   Found libavdevice, version 59.7.100
--   Found libavfilter, version 8.44.100
--   Found libavformat, version 59.27.100
--   Found libavcodec, version 59.37.100
--   Found libswresample, version 4.7.100
--   Found libswscale, version 6.7.100
--   Found libavutil, version 57.28.100
-- Configuring done
-- Generating done
-- Build files have been written to: /app/codechest/vaapi/vaapi-initialization/build
root@8691191c6ae4:/app/codechest/vaapi/vaapi-initialization/build# make
[ 50%] Building CXX object CMakeFiles/va_main.dir/va_main.cpp.o
[100%] Linking CXX executable va_main
[100%] Built target va_main
root@8691191c6ae4:/app/codechest/vaapi/vaapi-initialization/build# ./va_main 
Running drmFd
Running vaGetDisplayDRM
libva info: VA-API version 1.19.0
libva info: Trying to open /usr/lib/x86_64-linux-gnu/dri/iHD_drv_video.so
libva info: Found init function __vaDriverInit_1_19
libva info: va_openDriver() returns 0
Running vaTerminate
root@8691191c6ae4:/app/codechest/vaapi/vaapi-initialization/build# 
```