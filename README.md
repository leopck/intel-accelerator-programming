# Intel GPGPU and Media programming

Welcome to Intel GPGPU and Media programming! This repository is a rich collection of samples and boilerplates, primarily focusing on SYCL, VAAPI, and CMake. With the inclusion of Dockerfiles, our aim is to ensure reproducibility and ease of setup for developers. Whether you are looking to get started or seeking references for advanced applications, CodeChest is here to help!

## Directory Structure

- **cmake-executables**: Contains boilerplate for setting up executables using CMake.
- **cmake-library**: CMake setup for creating a library.
- **cmake-oneapi-sourcing**: Boilerplate for sourcing oneAPI environment within a CMake project.
- **dpcpp-host-matmul**: Sample of matrix multiplication using Data Parallel C++ (DPC++) on the host.
- **dpcpp-sycl-math**: Demonstrative implementation of mathematical operations using SYCL.

Each major directory, especially those containing sample applications, has its own README.md detailing the specifics of that particular example, along with a Dockerfile to ensure consistent environment setup.

## Getting Started

### Dependencies

Ensure you have Docker installed on your machine to take full advantage of the reproducible environments.

### Setup & Run

1. Clone the repository:
    ```bash
    git clone https://github.com/username/codechest.git
    cd codechest
    ```

2. Navigate to the desired directory:
    ```bash
    cd dpcpp-host-matmul
    ```

3. Build the Docker image (This step might vary based on the directory, but here's a general approach):
    ```bash
    docker build -t codechest-matmul .
    ```

4. Run the Docker container:
    ```bash
    docker run --rm -it codechest-matmul
    ```

5. Inside the Docker container, you can now run the sample application or explore further!

## Contributing

We welcome contributions! Please follow the usual GitHub pull request process:
1. Fork the repo.
2. Create a new feature branch.
3. Make your changes.
4. Submit a PR.

Ensure that you update the README.md in any directories you modify or add.

## License

This project is open-sourced under the MIT License. See the LICENSE file for more information.

## Feedback

If you have any feedback, issues, or feature requests, please post them in the [issues section](https://github.com/username/codechest/issues) of this repository.

---

Happy Coding! 🚀🔥

Note: Make sure to replace `username` with the appropriate GitHub username owning the repository.
