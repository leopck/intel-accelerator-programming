## VAAPI Sample: Create and Fill VASurface with Red Color

This sample demonstrates how to use the Video Acceleration API (VAAPI) to create a VASurface, fill it with the color red using `vaMapBuffer`, and then verify its content.

### Dependencies:
- `libavcodec`
- `libavformat`
- `libavutil`
- `libva` and `libva-drm`

### Usage:

1. Compile the code:
```
mkdir build
cd build
cmake ..
make
```
2. Run the compiled binary:
```
./va_main
```

### Description:

The sample begins by initializing the VADisplay from a DRM file descriptor. After successful initialization, a VASurface is created with the RGBA format using `vaCreateSurfaces`. This surface is then mapped to a VAImage using `vaDeriveImage`. This derived image provides a buffer that can be mapped into memory using `vaMapBuffer` to access or modify the pixel-level data. The VAImage pixel buffer is then filled with the color red. After filling, the sample verifies if the pixels in the image buffer are indeed red.

### Deep Dive:

1. **VASurfaceAttrib**:

    - `VASurfaceAttrib` is a structure that allows users to specify additional attributes or parameters during the creation of surfaces. In our sample, we use this structure to indicate our desired pixel format (RGBA).

    Parameters used:
    - `type`: Set to `VASurfaceAttribPixelFormat`, which indicates that we are specifying a desired pixel format.
    - `flags`: Set to `VA_SURFACE_ATTRIB_SETTABLE` to indicate that this attribute can be set.
    - `value.type`: Indicates the type of the attribute value. Set to `VAGenericValueTypeInteger` because the pixel format is represented as an integer constant.
    - `value.value.i`: Set to `VA_FOURCC_RGBA` to specify that we want an RGBA format.

The chosen values allow us to create a VASurface in the RGBA format, which is a common format for representing colored images with an alpha channel.

2. **vaCreateSurfaces vs. vaCreateImage**:

   - `vaCreateSurfaces`: This function is used to create one or more surfaces in the desired format. In VAAPI, surfaces are primarily used for operations like decoding, encoding, and various video processing tasks. In this sample, it's used to create a surface in the RGBA format, which will eventually be filled with the red color.
   
   - `vaCreateImage`: This function is used to create an image object that can be associated with a surface. The primary purpose of this image object is to provide direct access to the raw pixel data of the associated surface. However, in this sample, we don't directly use `vaCreateImage`; instead, we derive an image from our created surface.
   
3. **Why vaDeriveImage is required**:

   - `vaDeriveImage`: This function derives an image from a surface. The derived image provides direct access to the underlying pixel data of the surface without having to copy or convert the data. It's a more efficient way to access the pixel data of a surface when compared to creating a separate image and then associating it with the surface. In this sample, `vaDeriveImage` is used to get an image from our RGBA surface, which we then map into memory to fill with the color red.

### Notes:

While surfaces are used for high-level operations like decoding and encoding, images (whether created directly or derived from surfaces) are essential when direct pixel-level access or modifications are required.

---