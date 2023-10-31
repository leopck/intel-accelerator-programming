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

// Allocate Unified Shared Memory (USM)
void* allocateUSM(ze_context_handle_t context, ze_device_handle_t device, size_t size) {
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

void writeToUSM(ze_context_handle_t context, ze_device_handle_t device, void* usmMemory, const void* srcData, size_t dataSize){
    // Create command queue
    ze_command_queue_desc_t queueDesc = {};
    queueDesc.mode = ZE_COMMAND_QUEUE_MODE_DEFAULT;
    ze_command_queue_handle_t cmdQueue;
    ze_result_t result = zeCommandQueueCreate(context, device, &queueDesc, &cmdQueue);
    if (result != ZE_RESULT_SUCCESS) {
        std::cerr << "Failed to create command queue. Error code: " << result << std::endl;
        return;
    }

    // Create command list
    ze_command_list_desc_t cmdListDesc = {};
    ze_command_list_handle_t cmdList;
    result = zeCommandListCreate(context, device, &cmdListDesc, &cmdList);
    if (result != ZE_RESULT_SUCCESS) {
        std::cerr << "Failed to create command list. Error code: " << result << std::endl;
        zeCommandQueueDestroy(cmdQueue);
        return;
    }

    // Append memory copy command
    result = zeCommandListAppendMemoryCopy(cmdList, usmMemory, srcData, dataSize, nullptr, 0, nullptr);
    if (result != ZE_RESULT_SUCCESS) {
        std::cerr << "Failed to append memory copy. Error code: " << result << std::endl;
        zeCommandListDestroy(cmdList);
        zeCommandQueueDestroy(cmdQueue);
        return;
    }

    // Close the command list
    result = zeCommandListClose(cmdList);
    if (result != ZE_RESULT_SUCCESS) {
        std::cerr << "Failed to close command list. Error code: " << result << std::endl;
        zeCommandListDestroy(cmdList);
        zeCommandQueueDestroy(cmdQueue);
        return;
    }

    // Execute the command list
    result = zeCommandQueueExecuteCommandLists(cmdQueue, 1, &cmdList, nullptr);
    if (result != ZE_RESULT_SUCCESS) {
        std::cerr << "Failed to execute command list. Error code: " << result << std::endl;
    }

    // Clean up
    zeCommandListDestroy(cmdList);
    zeCommandQueueDestroy(cmdQueue);
}

// Fill the VASurface with a specific color (red)
void fillSurfaceWithRed(VADisplay va_dpy, VASurfaceID surface, int width, int height, ze_context_handle_t contextHandle, ze_device_handle_t deviceHandle) {
    VASurfaceStatus status;
    vaQuerySurfaceStatus(va_dpy, surface, &status);

    if (status != VASurfaceReady) {
        throw std::runtime_error("Surface not ready!");
        return;
    }

    // Define a red RGBA color
    std::vector<uint8_t> redData(width * height * 4, 0);
    for (size_t i = 0; i < redData.size(); i += 4) {
        redData[i] = 255;  // Red channel
    }

    // Map the surface to fill it
    VAImage vaImage;
    vaDeriveImage(va_dpy, surface, &vaImage);

    uint8_t* pBuf = nullptr;
    vaMapBuffer(va_dpy, vaImage.buf, (void**)&pBuf);
    memcpy(pBuf, redData.data(), redData.size());
    vaUnmapBuffer(va_dpy, vaImage.buf);

    vaDestroyImage(va_dpy, vaImage.image_id);

    // Synchronize after filling the surface
    ze_command_list_handle_t cmdList;
    ze_command_list_desc_t cmdListDesc = {};
    zeCommandListCreate(contextHandle, deviceHandle, &cmdListDesc, &cmdList);

    zeCommandListAppendBarrier(cmdList, nullptr, 0, nullptr);
    zeCommandListClose(cmdList);

    ze_command_queue_handle_t cmdQueue;
    ze_command_queue_desc_t cmdQueueDesc = {};
    zeCommandQueueCreate(contextHandle, deviceHandle, &cmdQueueDesc, &cmdQueue);

    zeCommandQueueExecuteCommandLists(cmdQueue, 1, &cmdList, nullptr);
    zeCommandQueueSynchronize(cmdQueue, UINT32_MAX);

    zeCommandListReset(cmdList);
    zeCommandListDestroy(cmdList);
    zeCommandQueueDestroy(cmdQueue);
}

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
    std::cout << "Running allocateUSM" << std::endl;
    void* usmMemory = allocateUSM(contextHandle, deviceHandle, memorySize);
    std::vector<int> srcData(memorySize / sizeof(int), 0);  // Assuming memorySize is a multiple of sizeof(int)
    for (int i = 0; i < 100 && i < srcData.size(); ++i) {
        srcData[i] = i + 1;
    }
    writeToUSM(contextHandle, deviceHandle, usmMemory, srcData.data(), memorySize);

    std::cout << "Running usmToDmaBuf" << std::endl;
    int dmaBufFd = usmToDmaBuf(contextHandle, usmMemory);
    std::cout << "USM DMA BUF FD: " << dmaBufFd << std::endl;
    std::cout << "Running DmaBufToVaSurface" << std::endl;
    VASurfaceID vaSurface = DmaBufToVaSurface(vaDisplay, (uintptr_t)dmaBufFd, width, height);
    verifyVASurface(vaSurface, vaDisplay);

    fillSurfaceWithRed(vaDisplay, vaSurface, width, height, contextHandle, deviceHandle);
    std::cout << "USM DMA BUF FD: " << dmaBufFd << std::endl;
    if (isSurfaceRed(vaDisplay, vaSurface, width, height)) {
        std::cout << "Surface is correctly filled with red!" << std::endl;
    } else {
        std::cerr << "Surface color doesn't match expected red color!" << std::endl;
    }
    std::cout << "Running vaTerminate" << std::endl;
    vaTerminate(vaDisplay);
    close(drmFd);
    std::cout << "Running zeMemFree" << std::endl;
    zeMemFree(contextHandle, usmMemory);
    std::cout << "Running zeContextDestroy" << std::endl;
    zeContextDestroy(contextHandle);
    return 0;
}