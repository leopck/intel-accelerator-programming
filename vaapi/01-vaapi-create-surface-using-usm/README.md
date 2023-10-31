# Creating VAAPI surfaces using USM
# Level Zero, VA-API, and DRM Integration

This repository showcases an integration between Intel's Level Zero API, Linux's Video Acceleration API (VA-API), and the Direct Rendering Manager (DRM). The code demonstrates the capability of converting Unified Shared Memory (USM) into a DMA BUF file descriptor, which is subsequently used with VA-API to create and manipulate surfaces.

## ASCII Diagram
```
USM Memory → DMA BUF → VA-API Surface
      ↑          ↑            ↑
Level Zero     DRM       VA-API & DRM Node
```

## Functions Overview

### 1. **zeDevicePciGetPropertiesExt()**
- **Purpose**: Fetch PCI properties of a given Level Zero device.
- **Parameters**:
  - `deviceHandle`: Handle of the Level Zero device.
  - `pciProperties`: Pointer to a structure to store PCI properties.

### 2. **supportsDmaBuf()**
- **Purpose**: Check if the Level Zero device supports DMA BUF.
- **Parameters**:
  - `deviceHandle`: Handle of the Level Zero device.

### 3. **createContext()**
- **Purpose**: Create a Level Zero context.
- **Parameters**:
  - `driverHandle`: Handle of the Level Zero driver.
  
### 4. **open() for DRM Node**
- **Purpose**: Open a specific render node for DRM.
- **Parameters**:
  - `path`: Path to the DRM render node (e.g., `/dev/dri/renderD128`).

### 5. **vaGetDisplayDRM()**
- **Purpose**: Fetch the VADisplay handle for the provided DRM FD.
- **Parameters**:
  - `drmFd`: File descriptor of the opened DRM node.

### 6. **allocateUSM()**
- **Purpose**: Allocate memory using Level Zero's USM mechanism.
- **Parameters**:
  - `contextHandle`: Handle of the Level Zero context.
  - `deviceHandle`: Handle of the Level Zero device.
  - `memorySize`: Size of memory to be allocated.
- **How it Works**: 
  - Unified Shared Memory (USM) in Level Zero provides a way to allocate memory that can be shared between the CPU and GPU without the need for manual copying or synchronization. This function allocates a piece of USM memory that can be accessed both by the host (CPU) and the device (GPU).
  
- **Parameters**:
  - `contextHandle`: Represents the Level Zero context. A context in GPU programming often represents a "world" in which memory allocations, kernels (or shaders), and command queues exist. It’s necessary for memory allocation in most GPU APIs.
  
  - `deviceHandle`: This specifies the GPU device where the memory is to be allocated. Different devices might have different properties or capabilities.
  
  - `memorySize`: The amount of memory to allocate. Here, it’s chosen to represent an image of resolution 1920x1080 with 4 bytes per pixel (RGBA format). This is a standard full HD resolution and the RGBA format is widely used because it represents Red, Green, Blue, and Alpha (transparency) channels.

### 7. **usmToDmaBuf()**
- **Purpose**: Convert USM memory to a DMA BUF file descriptor.
- **Parameters**:
  - `contextHandle`: Handle of the Level Zero context.
  - `usmMemory`: Pointer to the USM memory.
- **How it Works**: 
  - This function converts the previously allocated USM memory into a DMA BUF file descriptor. DMA BUF is a Linux mechanism that allows sharing memory buffers between multiple devices without actually copying the data. It’s especially useful in graphics workflows where one might need to share data between different hardware blocks, like a GPU and a video encoder. 
  
- **Parameters**:
  - `contextHandle`: Just like in `allocateUSM()`, the context in which the USM memory was allocated is required.
  
  - `usmMemory`: The pointer to the USM memory to be converted. This is the memory chunk that you want to share across devices or APIs.

### 8. **DmaBufToVaSurface()**
- **Purpose**: Create a VA-API surface using a DMA BUF FD.
- **Parameters**:
  - `vaDisplay`: Handle to the VA display.
  - `dmaBufFd`: DMA BUF file descriptor.
  - `width`: Width of the surface.
  - `height`: Height of the surface.
- **How it Works**: 
  - Once we have a DMA BUF descriptor, we can use it with the VA-API to create a surface. A "surface" in VA-API typically represents a chunk of GPU memory that can be used for various video operations, like encoding, decoding, or post-processing. In this function, the DMA BUF FD is used to create a VA-API surface without copying the data from the original USM memory.
  
- **Parameters**:
  - `vaDisplay`: This is the VA-API handle to the display, which is essential to perform any operations using the VA-API.
  
  - `dmaBufFd`: The DMA BUF file descriptor that represents the shared memory. This is what connects our Level Zero USM allocation with VA-API.
  
  - `width` and `height`: These specify the dimensions of the VA-API surface. It's essential that these match the original dimensions for which the memory was allocated (in our case, a 1920x1080 image). If there's a mismatch, it may result in undefined behavior or errors.

### 9. **fillSurfaceWithRed() and isSurfaceRed()**
- **Purpose**: Fill a VA-API surface with red color and verify it.
- **Parameters**:
  - `vaDisplay`: Handle to the VA display.
  - `vaSurface`: ID of the VA surface.
  - `width`: Width of the surface.
  - `height`: Height of the surface.

## Critical Parameters
1. **`/dev/dri/renderD128`**: This is a path to the DRM render node. It is device-specific and represents GPU nodes for rendering in Linux. Ensure your system has a GPU available at the chosen node path.
2. **USM Memory Size**: The code allocates memory with a size of 1920 x 1080 x 4 (which is equal to a full HD resolution image with 4 bytes per pixel). Adjust this as per your requirements.

## Rationale Behind Parameter Choices:
1. **USM**: The choice to use USM stems from its capability to seamlessly share memory between the host and device, thus simplifying data sharing and reducing the overhead of data copying.

2. **DMA BUF**: Using DMA BUF is a standard approach on Linux systems to share data between different devices or drivers. It ensures zero-copy sharing, which is highly efficient.

3. **VA-API Surface**: It represents a handle to GPU memory suitable for video operations. By using a DMA BUF descriptor to create a VA-API surface, we can manipulate the original USM memory using video processing capabilities provided by VA-API.

## Conclusion

The code provides a clear demonstration of how different Linux GPU APIs and mechanisms can work together seamlessly. Make sure to adapt paths and parameters as per your system's configuration and requirements.
