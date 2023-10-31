WIP

### VAAPI-OpenCL Interoperation:

1. **Memory Sharing**:
    - OpenCL's interoperation with VAAPI is facilitated through extensions, specifically the `CL_VA_API_MEDIA_SHARING_INTEL` extension for Intel platforms.
    - This extension allows VAAPI media surfaces to be shared with OpenCL without the need for explicit copying.
    - Under the hood, the OpenCL runtime and VAAPI use shared memory regions, allowing both to access the same physical memory. This is achieved without copying data between the two APIs.

2. **Synchronization**:
    - The OpenCL extension provides mechanisms to ensure synchronization when accessing shared surfaces. 
    - When creating the interop context, the property `CL_CONTEXT_INTEROP_USER_SYNC` determines who is responsible for synchronization:
        - `CL_TRUE`: The user (developer) is responsible for ensuring synchronization.
        - `CL_FALSE`: The OpenCL runtime handles synchronization.
    - OpenCL provides functions like `clEnqueueAcquireVA_APIMediaSurfacesINTEL` and `clEnqueueReleaseVA_APIMediaSurfacesINTEL` to explicitly handle acquisition and release of shared surfaces, ensuring proper synchronization.

### Implementing for Level Zero (L0):

1. **Memory Sharing**:
    - L0 doesn't have a direct mechanism for VAAPI interop like OpenCL does. However, since both VAAPI and L0 can interact with DMA-BUF on Linux, this provides a potential pathway.
    - The process would involve:
        1. Exporting a VAAPI surface to a DMA-BUF handle.
        2. Importing the DMA-BUF handle in L0 to create a shared buffer.
    - This way, both VAAPI and L0 can operate on the same physical memory.

2. **Synchronization**:
    - Since L0 doesn't natively understand VAAPI operations, manual synchronization mechanisms are essential.
    - After VAAPI operations on a buffer, use VAAPI's synchronization functions (like `vaSyncSurface`) to ensure operations are complete.
    - Before L0 operations on the same buffer, use L0's event or fence mechanisms to ensure prior L0 operations on the buffer are complete.
    - After L0 operations, before handing back to VAAPI, ensure L0 operations are complete using L0's synchronization primitives.
    - This ensures proper ordering and synchronization of operations between VAAPI and L0 on shared surfaces.

### Pseudo-Code Implementation for L0:

```cpp
// 1. Export VAAPI surface to DMA-BUF
int dma_buf_fd = vaExportSurfaceToDmaBuf(va_display, va_surface);

// 2. Import DMA-BUF to L0
ze_image_desc_t image_desc = {/*... setup image descriptor ...*/};
ze_image_handle_t l0_image;
zeImportImage(l0_context, &image_desc, dma_buf_fd, &l0_image);

// 3. After VAAPI operations, sync the surface
vaSyncSurface(va_display, va_surface);

// 4. Execute L0 operations on the image (l0_image)

// 5. After L0 operations, before handing back to VAAPI, ensure L0 operations are done
// Use L0's event or fence mechanisms for synchronization
```

Note: The above pseudo-code is a simplification to illustrate the concept. Actual implementation details might differ, and you'd need to consult the L0 and VAAPI documentation and headers for exact function signatures and structures.

This approach should provide a pathway for VAAPI-L0 interoperation, leveraging shared memory through DMA-BUF and manual synchronization techniques.