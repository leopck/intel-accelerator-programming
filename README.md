# Intel Accelerator Programming

This repository contains various examples and projects related to GPGPU and Intel's various accelerator programming using Intel technologies and platforms.

## Structure

- **cmake/**: Contains CMake configuration files for different scenarios.
  - **cmake-executables/**: CMake setup for executables.
  - **cmake-library/**: CMake setup for library projects.
  - **cmake-oneapi-sourcing/**: CMake configuration for projects using Intel OneAPI.

- **dpcpp/**: Contains projects using the Data Parallel C++ (DPC++) language.
  - **dpcpp-esimd/**: Matrix multiplication using explicit SIMD.
  - **dpcpp-host-matmul/**: Host matrix multiplication using DPC++.
  - **dpcpp-sycl-math/**: Matrix multiplication using SYCL.

- **level-zero/**: Contains projects leveraging the Level Zero API.
  - **l0-initialization/**: Basic initialization using the Level Zero API.

- **vaapi/**: Contains projects demonstrating the use of the Video Acceleration API (VAAPI).
  - **01-vaapi-create-surface-using-*/**: Different methods to create VAAPI surfaces.
  - **02-vaapi-ffmpeg-decoding/**: Decode using FFmpeg with VAAPI.
  - **03-vaapi-pipeline-*/**: VAAPI pipelines with various configurations.
  - **05-vaapi-interop-*/**: Interoperability examples between VAAPI and different technologies.

## Getting Started

1. Clone the repository:
   ```bash
   git clone https://github.com/leopck/intel-gpgpu-programming
   cd intel-gpgpu-programming
   ```

2. Navigate to the desired directory.
   
3. Follow the README instructions specific to each project.

## Requirements

- Intel OneAPI toolkit
- Modern C++ compiler
- CMake 3.x or higher
