#include <optional>
#include <vector>
#include <string>
#include <iostream>
#include <cstring>
#include <fcntl.h>
#include <unistd.h>
#include <fstream>

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

void save_to_rgb_file(const std::string& filename, uint32_t* pixels, int width, int height) {
    std::ofstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Failed to open file: " << filename << std::endl;
        return;
    }

    for (int i = 0; i < width * height; i++) {
        uint32_t pixel = pixels[i];
        uint8_t r = (pixel >> 16) & 0xFF; // Extract red channel
        uint8_t g = (pixel >> 8) & 0xFF;  // Extract green channel
        uint8_t b = pixel & 0xFF;         // Extract blue channel
        file.write((char*)&r, sizeof(r));
        file.write((char*)&g, sizeof(g));
        file.write((char*)&b, sizeof(b));
    }

    file.close();
    std::cout << "Saved RGB data to: " << filename << std::endl;
}

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

    // Map the VASurface to an image
    VAImage image;
    vaDeriveImage(vaDisplay, surface, &image);

    // Fill the image data with red
    uint32_t* pixels = nullptr;
    vaStatus = vaMapBuffer(vaDisplay, image.buf, (void**)&pixels);
    for (int i = 0; i < image.width * image.height; i++) {
        pixels[i] = RED_COLOR;
    }
    save_to_rgb_file("output.rgb", pixels, image.width, image.height); // Save to raw RGB file
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
    vaDestroySurfaces(vaDisplay, &surface, 1);
    std::cout << "Running vaTerminate" << std::endl;
    vaTerminate(vaDisplay);
    close(drmFd);
    return 0;
}