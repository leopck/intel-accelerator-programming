#include <optional>
#include <vector>
#include <string>
#include <iostream>
#include <cstring>
#include <fcntl.h>
#include <unistd.h>

#include <drm_fourcc.h> // For DRM_FORMAT_MOD_LINEAR

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/hwcontext.h>
#include <libavutil/hwcontext_vaapi.h>
#include <libavutil/pixdesc.h>
#include <va/va.h>
#include <va/va_drm.h>
#include <va/va_drmcommon.h>
}

#define RED_COLOR 0x00FF0000 // This represents the color red in ARGB format.

int main() {
    std::cout << "Running drmFd" << std::endl;
    int drmFd = open("/dev/dri/renderD128", O_RDWR); // Opening the first render node. Change index as per your system.
    if (drmFd < 0) {
        // Handle error
        return -1;
    }

    std::cout << "Running vaGetDisplayDRM" << std::endl;
    VADisplay vaDisplay = vaGetDisplayDRM(drmFd);
    if (vaDisplay == nullptr) {
        // Handle error
        close(drmFd);
        return -1;
    }
    int major, minor;
    VAStatus vaStatus = vaInitialize(vaDisplay, &major, &minor);
    if (vaStatus != VA_STATUS_SUCCESS) {
        // Handle error
        vaTerminate(vaDisplay);
        close(drmFd);
        return -1;
    }

    // 1. Create a VASurface
    // Set up VASurface attributes for RGBA format
    VASurfaceAttrib attrib;
    attrib.type = VASurfaceAttribPixelFormat;
    attrib.flags = VA_SURFACE_ATTRIB_SETTABLE;
    attrib.value.type = VAGenericValueTypeInteger;
    attrib.value.value.i = VA_FOURCC_RGBA;

    // Create the VASurface with RGBA format
    VASurfaceID surface;
    VAStatus vaStatusSurface = vaCreateSurfaces(vaDisplay, VA_RT_FORMAT_RGB32, 1920, 1080, &surface, 1, &attrib, 1);
    if (vaStatusSurface != VA_STATUS_SUCCESS) {
        // Handle error
        vaTerminate(vaDisplay);
        close(drmFd);
        return -1;
    }
    
    // 2. Set up VAImageFormat for RGBA format
    VAImageFormat imageFormat = {};
    imageFormat.fourcc = VA_FOURCC_RGBA;
    imageFormat.byte_order = VA_LSB_FIRST; // Most common for little-endian systems
    imageFormat.bits_per_pixel = 32;

    // Create an image associated with the format
    VAImage image;
    VAStatus vaStatusImage = vaCreateImage(vaDisplay, &imageFormat, 1920, 1080, &image);
    if (vaStatusImage != VA_STATUS_SUCCESS) {
        // Handle error
        vaDestroySurfaces(vaDisplay, &surface, 1);
        vaTerminate(vaDisplay);
        close(drmFd);
        return -1;
    }
    // Fill the image data with red
    uint32_t* pixels = nullptr;
    vaStatus = vaMapBuffer(vaDisplay, image.buf, (void**)&pixels);
    for (int i = 0; i < image.width * image.height; i++) {
        pixels[i] = RED_COLOR;
    }
    vaUnmapBuffer(vaDisplay, image.buf);

    // Verification
    bool isRed = true;
    for (int i = 0; i < image.width * image.height; i++) {
        if (pixels[i] != RED_COLOR) {
            isRed = false;
            break;
        }
    }

    if (isRed) {
        std::cout << "Surface has been filled with red!" << std::endl;
    } else {
        std::cout << "Surface filling verification failed!" << std::endl;
    }

    // Cleanup
    vaDestroyImage(vaDisplay, image.image_id); // Destroy the VAImage
    vaDestroySurfaces(vaDisplay, &surface, 1);
    std::cout << "Running vaTerminate" << std::endl;
    vaTerminate(vaDisplay);
    close(drmFd);
    return 0;
}