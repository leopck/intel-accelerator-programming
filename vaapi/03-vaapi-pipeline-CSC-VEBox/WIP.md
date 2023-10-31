WIP

```c
VASurfaceID create_surface_rgbx(uint32_t width, uint32_t height) {
    // Packed
    int32_t fourcc = VA_FOURCC_RGBA;
    uint32_t format = VA_RT_FORMAT_RGB32;
    
    VASurfaceAttrib sattr = {};
    sattr.type = VASurfaceAttribPixelFormat;
    sattr.flags = VA_SURFACE_ATTRIB_SETTABLE;
    sattr.value.type = VAGenericValueTypeInteger;
    sattr.value.value.i = fourcc;

    VASurfaceID va_surface = VA_INVALID_SURFACE;
    VAStatus status = vaCreateSurfaces(va_display_, format, width, height,
        &va_surface, 1, &sattr, 1);
    if (status != VA_STATUS_SUCCESS)
        throw std::runtime_error("vaCreateSurfaces() failed: " + std::to_string(status));
    
    return va_surface;
}

VASurfaceID convert_surface_to_rgb(VASurfaceID surface_nv12) {
    uint16_t w = 0, h = 0;
    {
        VAImage image;
        
        VAStatus status = vaDeriveImage(va_display_, surface_nv12, &image);
        if (status != VA_STATUS_SUCCESS)
            throw std::runtime_error("vaDeriveImage() failed: " + std::to_string(status));

        w = image.width;
        h = image.height;

        status = vaDestroyImage(va_display_, image.image_id);
    }

    VARectangle region{.x = 0, .y = 0, .width = w, .height = h};
    
    VAProcPipelineParameterBuffer pipe_param = {};
    pipe_param.surface = surface_nv12;

    VABufferID pipe_buf_id = VA_INVALID_ID;
    VAStatus status = vaCreateBuffer(va_display_, va_context_id_, VAProcPipelineParameterBufferType,
        sizeof(pipe_param), 1, &pipe_param, &pipe_buf_id);
    if (status != VA_STATUS_SUCCESS)
        throw std::runtime_error("vaCreateBuffer() failed: " + std::to_string(status));

    // TODO: proper surface and buffer deletion in case of exception

    auto dst_surface = create_surface_rgbx(w, h);
    status = vaBeginPicture(va_display_, va_context_id_, dst_surface);
    if (status != VA_STATUS_SUCCESS)
        throw std::runtime_error("vaBeginPicture() failed: " + std::to_string(status));

    status = vaRenderPicture(va_display_, va_context_id_, &pipe_buf_id, 1);
    if (status != VA_STATUS_SUCCESS)
        throw std::runtime_error("vaRenderPicture() failed: " + std::to_string(status));
    
    status = vaEndPicture(va_display_, va_context_id_);
    if (status != VA_STATUS_SUCCESS)
        throw std::runtime_error("vaEndPicture() failed: " + std::to_string(status));

    // Optional: wait for surface to be ready
    // status = vaSyncSurface(va_display_, dst_surface);
    // if (status != VA_STATUS_SUCCESS)
    //     throw std::runtime_error("vaSyncSurface() failed: " + std::to_string(status));

    status = vaDestroyBuffer(va_display_, pipe_buf_id);
    if (status != VA_STATUS_SUCCESS)
        throw std::runtime_error("vaDestroyBuffer() failed: " + std::to_string(status));

    return dst_surface;
}
```