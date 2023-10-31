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
#include <level_zero/ze_api.h>

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

typedef struct {
    unsigned int width;
    unsigned int height;
    ptrdiff_t stride;
    ptrdiff_t offset;
    VASurfaceID va_surface;
    VADisplay va_display;
    void* usm_ptr;
    ze_context_handle_t ze_context;
} Frame;

// Initialize Level Zero driver
ze_driver_handle_t initializeDriver() {
    uint32_t driverCount = 0;

    // Ensure Level Zero is initialized
    ze_result_t result = zeInit(ZE_INIT_FLAG_GPU_ONLY);
    if (result != ZE_RESULT_SUCCESS) {
        throw std::runtime_error("Failed to initialize Level Zero");
    }
    
    // Query the number of available drivers
    result = zeDriverGet(&driverCount, nullptr);
    if (result != ZE_RESULT_SUCCESS) {
        throw std::runtime_error("Error querying driver count");
    }
    if (driverCount == 0) {
        throw std::runtime_error("Failed to find any drivers"); // No drivers found
    }
    
    // Allocate space for driver handles
    ze_driver_handle_t* drivers = new ze_driver_handle_t[driverCount];
    
    // Retrieve the driver handles
    result = zeDriverGet(&driverCount, drivers);
    if (result != ZE_RESULT_SUCCESS) {
        delete[] drivers;
        throw std::runtime_error("Error retrieving driver handles");
    }
    
    // For each driver, get the device count and print it
    for (uint32_t i = 0; i < driverCount; ++i) {
        uint32_t deviceCount = 0;
        result = zeDeviceGet(drivers[i], &deviceCount, nullptr);
        if (result != ZE_RESULT_SUCCESS) {
            delete[] drivers;
            throw std::runtime_error("Error querying device count for driver " + std::to_string(i));
        }
        std::cout << "Driver " << i << " has " << deviceCount << " device(s)." << std::endl;
    }

    // Store the first driver handle and delete the dynamic array
    ze_driver_handle_t driverHandle = drivers[0];
    delete[] drivers;
    
    return driverHandle;
}

// Initialize Level Zero device
ze_device_handle_t initializeDevice(ze_driver_handle_t driverHandle) {
    uint32_t deviceCount = 0;

    // Query the number of available devices for the driver
    ze_result_t result = zeDeviceGet(driverHandle, &deviceCount, nullptr);
    if (result != ZE_RESULT_SUCCESS) {
        throw std::runtime_error("Error querying device count");
    }

    if (deviceCount == 0) {
        throw std::runtime_error("No devices found for the driver"); // No devices found
    }

    std::cout << "The driver has " << deviceCount << " device(s)." << std::endl;

    // Allocate space for device handles
    ze_device_handle_t* devices = new ze_device_handle_t[deviceCount];
    
    // Retrieve the device handles
    result = zeDeviceGet(driverHandle, &deviceCount, devices);
    if (result != ZE_RESULT_SUCCESS) {
        delete[] devices;
        throw std::runtime_error("Error retrieving device handles");
    }

    // Store the first device handle and delete the dynamic array
    ze_device_handle_t deviceHandle = devices[0];
    delete[] devices;
    
    return deviceHandle;
}

// Get Context
ze_context_handle_t createContext(ze_driver_handle_t driverHandle) {
    ze_context_desc_t contextDesc = {};
    ze_context_handle_t contextHandle;

    ze_result_t result = zeContextCreate(driverHandle, &contextDesc, &contextHandle);
    if(result != ZE_RESULT_SUCCESS) {
        throw std::runtime_error("Failed to create Level Zero context");
    }

    return contextHandle;
}

void* vaapi_to_usm(VASurfaceID va_surface, VADisplay va_display_, ze_context_handle_t ze_context_, ze_device_handle_t ze_device_) {
    // vaapi -> dmabuf conversion
    VADRMPRIMESurfaceDescriptor prime_desc;
    vaExportSurfaceHandle(
        va_display_, va_surface, VA_SURFACE_ATTRIB_MEM_TYPE_DRM_PRIME_2,
        VA_EXPORT_SURFACE_READ_WRITE,
        &prime_desc);
    
    // Assuming that the RGB plane is at index 0
    if (prime_desc.num_layers != 1 || prime_desc.layers->num_planes != 1) {
        throw std::runtime_error("Unexpected number of layers or planes in the descriptor.");
    }

    // Filling Frame
    Frame frame;
    frame.width = prime_desc.width;
    frame.height = prime_desc.height;
    frame.stride = prime_desc.layers->pitch[0];
    frame.offset = prime_desc.layers->offset[0];
    frame.va_surface = va_surface;
    frame.va_display = va_display_;

    auto dma_fd = prime_desc.objects->fd;
    auto dma_size = prime_desc.objects->size;
    
    // dmabuf -> USM conversion
    ze_external_memory_import_fd_t import_fd = {
        ZE_STRUCTURE_TYPE_EXTERNAL_MEMORY_IMPORT_FD,
        nullptr,
        ZE_EXTERNAL_MEMORY_TYPE_FLAG_DMA_BUF, dma_fd
    };
    ze_device_mem_alloc_desc_t alloc_desc = {};
    alloc_desc.stype = ZE_STRUCTURE_TYPE_DEVICE_MEM_ALLOC_DESC;
    alloc_desc.pNext = &import_fd;
    
    ze_result_t ze_res = zeMemAllocDevice(ze_context_, &alloc_desc, dma_size, 1, ze_device_, &frame.usm_ptr);
    if (ze_res != ZE_RESULT_SUCCESS) {
        std::cout << "[ERROR]: Failed to convert DMA to USM pointer: " << ze_res << std::endl;
    }

    frame.ze_context = ze_context_;
    close(dma_fd);
    return frame.usm_ptr;
}

int main() {
    // Initialize Level Zero driver and device
    std::cout << "Running initializeDriver" << std::endl;
    ze_driver_handle_t driverHandle = initializeDriver();
    if (!driverHandle) {
        std::cerr << "Failed to initialize the Level Zero driver!" << std::endl;
        return -1;
    }

    std::cout << "Running initializeDevice" << std::endl;
    ze_device_handle_t deviceHandle = initializeDevice(driverHandle);
    if (!deviceHandle) {
        std::cerr << "Failed to initialize the Level Zero device!" << std::endl;
        return -1;
    }

    // Get PCI BDF ( Bus, Device, Function ) from L0
    ze_pci_ext_properties_t pciProperties = {};

    ze_result_t result = zeDevicePciGetPropertiesExt(deviceHandle, &pciProperties);
    if(result == ZE_RESULT_SUCCESS) {
        ze_pci_address_ext_t pciAddress = pciProperties.address; // Extracting the PCI address from the properties
        printf("Level Zero GPU PCI Address: %04x:%02x:%02x.%x\n", pciAddress.domain, pciAddress.bus, pciAddress.device, pciAddress.function);
    }
    
    std::cout << "Running createContext" << std::endl;
    ze_context_handle_t contextHandle = createContext(driverHandle);
    if (!contextHandle) {
        std::cerr << "Failed to create the Level Zero context!" << std::endl;
        return -1;
    }

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

    void *usm = vaapi_to_usm(surface, vaDisplay, contextHandle, deviceHandle);
    vaTerminate(vaDisplay);
    close(drmFd);
    std::cout << "Running zeMemFree" << std::endl;
    zeMemFree(contextHandle, usm);
    std::cout << "Running zeContextDestroy" << std::endl;
    zeContextDestroy(contextHandle);
    return 0;
}