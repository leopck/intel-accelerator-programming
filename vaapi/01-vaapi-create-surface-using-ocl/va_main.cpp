#include <optional>
#include <vector>
#include <string>
#include <iostream>
#include <cstring>
#include <fstream>

#include <thread>
#include <chrono>

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
#define CL_TARGET_OPENCL_VERSION 220
#include <CL/cl.h>
#include <CL/cl_va_api_media_sharing_intel.h>

bool isSurfaceRed(VADisplay va_dpy, VASurfaceID surface, int width, int height) {
    VAImage vaImage;
    vaDeriveImage(va_dpy, surface, &vaImage);

    uint8_t* pBuf = nullptr;
    vaMapBuffer(va_dpy, vaImage.buf, (void**)&pBuf);

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            uint8_t* pixel = &pBuf[y * vaImage.pitches[0] + x * 4];
            if (pixel[0] != 255 || pixel[1] != 0 || pixel[2] != 0) {
                vaUnmapBuffer(va_dpy, vaImage.buf);
                vaDestroyImage(va_dpy, vaImage.image_id);
                return false;
            }
        }
    }

    vaUnmapBuffer(va_dpy, vaImage.buf);
    vaDestroyImage(va_dpy, vaImage.image_id);
    return true;
}
bool verifyVASurface(VASurfaceID surface, VADisplay vaDpy) {
    VAImage vaImg;
    VAStatus vaStatus;
    uint8_t* bufPtr = nullptr;

    // Derive an image from the surface
    vaStatus = vaDeriveImage(vaDpy, surface, &vaImg);
    if (vaStatus != VA_STATUS_SUCCESS) {
        std::cerr << "vaDeriveImage failed. Error code: " << vaStatus << std::endl;
        return false;
    }

    // Map the buffer into process address space
    vaStatus = vaMapBuffer(vaDpy, vaImg.buf, (void**)&bufPtr);
    if (vaStatus != VA_STATUS_SUCCESS) {
        std::cerr << "vaMapBuffer failed. Error code: " << vaStatus << std::endl;
        vaDestroyImage(vaDpy, vaImg.image_id); // Clean up the image
        return false;
    }
    bool crash_flag = true;
    // Check the data; let's assume the image data type is int32 and stride is width * 4
    for (int i = 0; i < 100; ++i) {
        if (((int*)bufPtr)[i] != i + 1) {
            std::cerr << "Data mismatch at index " << i << ". Expected: " << (i + 1) << ", Got: " << ((int*)bufPtr)[i] << std::endl;
            // vaUnmapBuffer(vaDpy, vaImg.buf); // Unmap before returning
            // vaDestroyImage(vaDpy, vaImg.image_id); // Clean up the image
            crash_flag = false;
        }
    }

    vaUnmapBuffer(vaDpy, vaImg.buf);
    vaDestroyImage(vaDpy, vaImg.image_id); // Clean up the image
    if (crash_flag == false)
        throw std::runtime_error("Surface not ready!");
    std::cout << "GPU data match!" << std::endl;
    return true; // Data matched
}
cl_context createOpenCLContext() {
    cl_int err;

    // Platform availability
    cl_uint numPlatforms;
    err = clGetPlatformIDs(0, nullptr, &numPlatforms);
    if (err != CL_SUCCESS || numPlatforms == 0) {
        throw std::runtime_error("No OpenCL platforms found");
    }

    std::vector<cl_platform_id> platforms(numPlatforms);
    err = clGetPlatformIDs(numPlatforms, platforms.data(), nullptr);
    if (err != CL_SUCCESS) {
        throw std::runtime_error("Error getting platform IDs");
    }

    // Just choosing the first platform for demonstration purposes
    cl_platform_id chosenPlatform = platforms[0];

    // Device Availability
    cl_uint numDevices;
    err = clGetDeviceIDs(chosenPlatform, CL_DEVICE_TYPE_GPU, 0, nullptr, &numDevices);
    if (err != CL_SUCCESS || numDevices == 0) {
        throw std::runtime_error("No OpenCL GPU devices found for the platform");
    }

    std::vector<cl_device_id> devices(numDevices);
    err = clGetDeviceIDs(chosenPlatform, CL_DEVICE_TYPE_GPU, numDevices, devices.data(), nullptr);
    if (err != CL_SUCCESS) {
        std::string errorMsg = "OpenCL Error: " + std::to_string(err);
        throw std::runtime_error(errorMsg);
    }

    cl_context context = clCreateContext(nullptr, numDevices, devices.data(), nullptr, nullptr, &err);
    if(err != CL_SUCCESS) {
        std::string errorMsg = "Failed to create OpenCL context. Error: " + std::to_string(err);
        throw std::runtime_error(errorMsg);
    }
    
    return context;
}

cl_mem createOpenCLBuffer(cl_context context, int width, int height) {
    cl_int err;
    cl_mem buffer = clCreateBuffer(context, CL_MEM_READ_WRITE, width * height * 4, NULL, &err);
    if(err != CL_SUCCESS) {
        throw std::runtime_error("Failed to create OpenCL buffer");
    }
    return buffer;
}
VASurfaceID OpenCLBufferToVaSurface(VADisplay va_dpy, cl_context clContext, cl_mem clBuffer, int width, int height) {
    VAStatus va_status;
    VASurfaceID va_surface;
    cl_int err;

    VASurfaceAttribExternalBuffers va_ext_buf_desc;
    VASurfaceAttrib attribs[2];

    attribs[0].type = VASurfaceAttribMemoryType;
    attribs[0].flags = VA_SURFACE_ATTRIB_SETTABLE;
    attribs[0].value.type = VAGenericValueTypeInteger;
    attribs[0].value.value.i = VA_SURFACE_ATTRIB_MEM_TYPE_VA;

    attribs[1].type = VASurfaceAttribExternalBufferDescriptor;
    attribs[1].flags = VA_SURFACE_ATTRIB_SETTABLE;
    attribs[1].value.type = VAGenericValueTypePointer;
    attribs[1].value.value.p = &va_ext_buf_desc;

    va_ext_buf_desc.pixel_format = VA_FOURCC_RGBA;
    va_ext_buf_desc.width = width;
    va_ext_buf_desc.height = height;
    va_ext_buf_desc.data_size = width * height * 4;
    va_ext_buf_desc.num_planes = 1;
    va_ext_buf_desc.pitches[0] = width * 4;
    va_ext_buf_desc.offsets[0] = 0;

    // Create a VASurface from the OpenCL buffer
    err = clGetMemObjectInfo(clBuffer, CL_MEM_VA_API_MEDIA_SURFACE_INTEL, sizeof(va_surface), &va_surface, NULL);
    if(err != CL_SUCCESS) {
        throw std::runtime_error("Failed to get VASurface from OpenCL buffer");
    }

    va_status = vaCreateSurfaces(va_dpy, VA_RT_FORMAT_RGB32, width, height, &va_surface, 1, attribs, 2);
    if(va_status != VA_STATUS_SUCCESS) {
        throw std::runtime_error("Failed to create VA surface from OpenCL buffer");
    }

    return va_surface;
}

int main() {

    std::cout << "Running drmFd" << std::endl;
    int drmFd = open("/dev/dri/renderD128", O_RDWR); // Opening the first render node. Change index as per your system.
    if (drmFd < 0) {
        std::cerr << "Failed to open DRM" << std::endl;
        return -1;
    }

    std::cout << "Running vaGetDisplayDRM" << std::endl;
    VADisplay vaDisplay = vaGetDisplayDRM(drmFd);
    if (vaDisplay == nullptr) {
        std::cerr << "Failed to get VA Display" << std::endl;
        close(drmFd);
        return -1;
    }

    int major, minor;
    VAStatus vaStatus = vaInitialize(vaDisplay, &major, &minor);
    if (vaStatus != VA_STATUS_SUCCESS) {
        std::cerr << "Failed to initialize VA Display" << std::endl;
        vaTerminate(vaDisplay);
        close(drmFd);
        return -1;
    }

    // Create OpenCL context
    cl_context clContext = createOpenCLContext();

    // Allocate memory using OpenCL buffer
    size_t width = 1920;
    size_t height = 1080;
    cl_mem clBuffer = createOpenCLBuffer(clContext, width, height);

    // Convert the OpenCL buffer to a VASurface
    VASurfaceID vaSurface = OpenCLBufferToVaSurface(vaDisplay, clContext, clBuffer, width, height);

    // Verify the VASurface
    if (!verifyVASurface(vaSurface, vaDisplay)) {
        std::cerr << "VASurface verification failed!" << std::endl;
        vaTerminate(vaDisplay);
        close(drmFd);
        return -1;
    }

    // Check if the surface is red
    if (isSurfaceRed(vaDisplay, vaSurface, width, height)) {
        std::cout << "Surface is correctly filled with red!" << std::endl;
    } else {
        std::cerr << "Surface color doesn't match expected red color!" << std::endl;
    }

    // Clean up
    vaTerminate(vaDisplay);
    close(drmFd);
    return 0;
}