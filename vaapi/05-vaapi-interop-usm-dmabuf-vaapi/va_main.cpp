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

VASurfaceID DmaBufToVaSurface(VADisplay va_dpy, uintptr_t dma_fd, int width, int height) {
    VAStatus va_status;
    VASurfaceID va_surface;

    // TODO: Check if PRIME_2 is supported
    bool use_prime2 = true;  // Assuming it's supported; adjust accordingly

    // Common attributes
    VASurfaceAttrib attribs[2];
    attribs[0].type = VASurfaceAttribMemoryType;
    attribs[0].flags = VA_SURFACE_ATTRIB_SETTABLE;
    attribs[0].value.type = VAGenericValueTypeInteger;

    attribs[1].type = VASurfaceAttribExternalBufferDescriptor;
    attribs[1].flags = VA_SURFACE_ATTRIB_SETTABLE;
    attribs[1].value.type = VAGenericValueTypePointer;

    if (use_prime2) {
        std::cout << "Using PRIME_2 to export the memory" << std::endl;
        VADRMPRIMESurfaceDescriptor prime_desc;

        // Set up the VASurfaceAttrib array
        attribs[0].value.value.i = VA_SURFACE_ATTRIB_MEM_TYPE_DRM_PRIME_2;
        attribs[1].value.value.p = &prime_desc;

        // Initialize the VADRMPRIMESurfaceDescriptor structure.
        prime_desc.fourcc = VA_FOURCC_RGBA; // Assuming RGBA format
        prime_desc.width = width;
        prime_desc.height = height;
        prime_desc.num_objects = 1;  // Assuming one object for simplicity
        prime_desc.objects[0].fd = dma_fd;
        prime_desc.objects[0].size = width * height * 4;  // For RGBA
        prime_desc.objects[0].drm_format_modifier = DRM_FORMAT_MOD_LINEAR; // Set as Linear memory because it's layout as linearly

        prime_desc.num_layers = 1;  // Assuming one layer for RGBA
        prime_desc.layers[0].drm_format = VA_FOURCC_RGBA; // Again, assuming RGBA format
        prime_desc.layers[0].num_planes = 1;  // RGBA is packed into a single plane
        prime_desc.layers[0].object_index[0] = 0;
        prime_desc.layers[0].offset[0] = 0;
        prime_desc.layers[0].pitch[0] = width * 4;  // For RGBA

    } else {
        std::cout << "Using PRIME to export the memory" << std::endl;
        VASurfaceAttribExternalBuffers va_ext_buf_desc;

        // Set up the VASurfaceAttrib array
        attribs[0].value.value.i = VA_SURFACE_ATTRIB_MEM_TYPE_DRM_PRIME;
        attribs[1].value.value.p = &va_ext_buf_desc;

        // Initialize the VASurfaceAttribExternalBuffers structure.
        va_ext_buf_desc.buffers = &dma_fd; // Pointer to the DMA-BUF FD
        va_ext_buf_desc.num_buffers = 1;
        va_ext_buf_desc.width = width;
        va_ext_buf_desc.height = height;
        va_ext_buf_desc.pixel_format = VA_FOURCC_RGBA; // Assuming the NV12 format
        va_ext_buf_desc.data_size = width * height * 4; // For RGBA
        va_ext_buf_desc.num_planes = 1; // RGBA (or RGBX) is packed into a single plane
        va_ext_buf_desc.pitches[0] = width * 4; // For RGBA (or RGBX)
        va_ext_buf_desc.offsets[0] = 0;

    }

    // Create the surface
    va_status = vaCreateSurfaces(
        va_dpy,
        VA_RT_FORMAT_RGB32,
        width,
        height,
        &va_surface,
        1,
        attribs,
        2 // Number of attribs
    );
    if (va_status != VA_STATUS_SUCCESS) {
        std::string errorMsg = use_prime2 ? "Failed to create vaCreateSurfaces with PRIME_2" : "Failed to create vaCreateSurfaces";
        throw std::runtime_error(errorMsg);
    }

    std::cout << "Successfully create a VASurface based on USM buffer" << std::endl;

    return va_surface;
}
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

    zeContextCreate(driverHandle, &contextDesc, &contextHandle);

    return contextHandle;
}

// Create Unified Shared Memory (USM)
void* createUSMmemory(ze_context_handle_t context, ze_device_handle_t device, size_t size) {
    ze_device_mem_alloc_desc_t allocDesc = {};
    allocDesc.ordinal = 0;
    allocDesc.flags = 0;
    
    void* memory = nullptr;
    ze_result_t result = zeMemAllocDevice(context, &allocDesc, size, 4096, device, &memory);
    
    if (result != ZE_RESULT_SUCCESS) {
        // Handle error
        std::cerr << "Failed to allocate USM memory. Error code: " << result << std::endl;
        return nullptr;
    }
    
    return memory;
}

// Convert USM to DMA BUF
int usmToDmaBuf(ze_context_handle_t context, void* usmPtr) {
    ze_external_memory_export_fd_t export_fd = {ZE_STRUCTURE_TYPE_EXTERNAL_MEMORY_EXPORT_FD, nullptr,
                                                ZE_EXTERNAL_MEMORY_TYPE_FLAG_DMA_BUF, 0};
    ze_memory_allocation_properties_t alloc_props = {};
    alloc_props.stype = ZE_STRUCTURE_TYPE_MEMORY_ALLOCATION_PROPERTIES;
    alloc_props.pNext = &export_fd;
    ze_result_t ze_res = zeMemGetAllocProperties(context, usmPtr, &alloc_props, nullptr);
    if (ze_res != ZE_RESULT_SUCCESS)
        throw std::runtime_error("Failed to convert USM pointer to DMA-BUF: " + std::to_string(ze_res));

    auto dma_fd = export_fd.fd;
    return dma_fd; // This is the DMA BUF handle
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

    // Allocate memory using USM
    size_t width = 1920;
    size_t height = 1080;
    size_t memorySize = width * height * 4;
    std::cout << "Running createUSMmemory" << std::endl;
    void* usmMemory = createUSMmemory(contextHandle, deviceHandle, memorySize);
    std::cout << "Running usmToDmaBuf" << std::endl;
    int dmaBufFd = usmToDmaBuf(contextHandle, usmMemory);
    std::cout << "USM DMA BUF FD: " << dmaBufFd << std::endl;
    std::cout << "Running DmaBufToVaSurface" << std::endl;

    // Create the vaSurface based on the USM memory
    VASurfaceID vaSurface = DmaBufToVaSurface(vaDisplay, (uintptr_t)dmaBufFd, width, height);
    vaTerminate(vaDisplay);
    close(drmFd);
    std::cout << "Running zeMemFree" << std::endl;
    zeMemFree(contextHandle, usmMemory);
    std::cout << "Running zeContextDestroy" << std::endl;
    zeContextDestroy(contextHandle);
    return 0;
}