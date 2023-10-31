# Level Zero and VAAPI Integration via DMA-BUF README

This code demonstrates the integration of Level Zero (a low-level GPU programming API) with VAAPI (the Video Acceleration API, used for hardware video decode/encode) via the DMA-BUF mechanism.

## Overview

1. Initialize Level Zero driver and device.
2. Create a Level Zero context.
3. Allocate Unified Shared Memory (USM) using Level Zero.
4. Convert the USM memory to a DMA BUF.
5. Use the DMA BUF to create a VAAPI surface.

## Diagram

```
                  +--------------+
                  | Level Zero   |
                  | +----------+ |
                  | |  USM    | |        DMA BUF
                  | +----------+ | <-------------------> +-------------+
                  +--------------+                      |   VAAPI     |
                                                        | +---------+ |
                                                        | | Surface | |
                                                        | +---------+ |
                                                        +-------------+
```

## Functions Explanation

### PRIME and DMA BUF Mechanism

- **DmaBufToVaSurface**: This function takes in the VAAPI display, a DMA BUF file descriptor, width, and height to create a VAAPI surface from the provided DMA BUF. It demonstrates how the memory allocated by Level Zero (and represented as a DMA BUF) can be used by other APIs that understand the DMA BUF mechanism.

  **Attributes and Parameters**:
  - **VASurfaceAttrib**: Attributes for creating VAAPI surfaces. The type of memory and external buffer descriptors are set in this attribute.
  - **VADRMPRIMESurfaceDescriptor**: A structure used with PRIME_2. It describes the memory layout and DRM properties of the DMA BUF.
  - **VASurfaceAttribExternalBuffers**: A structure used with PRIME. It describes the memory layout of the DMA BUF.

### Level Zero Initialization and Memory Management

- **initializeDriver**: This function initializes the Level Zero driver.
  
- **initializeDevice**: It initializes a Level Zero device given a driver handle.

- **createContext**: This function creates a Level Zero context using a driver handle.

- **createUSMmemory**: Allocates USM memory of a given size. Unified Shared Memory (USM) allows shared use of pointers between the host and the device. This is using `zeMemAllocDevice` to allocate the memory in the device directly, we can also allocate using host and shared (auto sync between device and host) but this case, I want to directly allocate on the device memory itself.

- **usmToDmaBuf**: Converts a USM pointer to a DMA BUF file descriptor. This is the bridge between Level Zero memory and the DMA BUF mechanism.

### Main Flow

In the `main()` function:

1. Level Zero driver and device are initialized.
2. The PCI address of the GPU is extracted.
3. A Level Zero context is created.
4. A DRM (Direct Rendering Manager) file descriptor is obtained by opening a render node.
5. A VAAPI display is created using the DRM FD.
6. Memory is allocated using Level Zero's USM mechanism.
7. The USM memory is converted to a DMA BUF.
8. A VAAPI surface is created using the DMA BUF.
9. All resources are cleaned up.

## Why use Level Zero with VAAPI?

Level Zero offers a direct, low-overhead access to GPU resources. When integrated with VAAPI, it can be used to efficiently manage GPU memory for video processing tasks, enabling high-performance video processing pipelines.

## Wrapping up

This integration showcases how modern graphics and compute APIs can be combined seamlessly, allowing for optimized workflows and direct control over GPU resources. It's a demonstration of the power and flexibility of low-level GPU programming in the context of video processing.