#include <CL/sycl.hpp>
#include <iostream>
#include <iomanip>

const int SIZE = 11;

void matrix_multiply(const float* A, const float* B, float* C, int size) {
    cl::sycl::queue q = sycl::queue(sycl::gpu_selector_v);

    cl::sycl::buffer<float, 1> bufferA(A, cl::sycl::range<1>(size * size));
    cl::sycl::buffer<float, 1> bufferB(B, cl::sycl::range<1>(size * size));
    cl::sycl::buffer<float, 1> bufferC(C, cl::sycl::range<1>(size * size));

    q.submit([&](cl::sycl::handler& cgh) {
        auto accA = bufferA.get_access<cl::sycl::access::mode::read>(cgh);
        auto accB = bufferB.get_access<cl::sycl::access::mode::read>(cgh);
        auto accC = bufferC.get_access<cl::sycl::access::mode::write>(cgh);

        cgh.parallel_for<class MatMul>(cl::sycl::range<2>(size, size), [=](cl::sycl::id<2> id) {
            int row = id[0];
            int col = id[1];
            float sum = 0.0f;
            for (int k = 0; k < size; ++k) {
                sum += accA[row * size + k] * accB[k * size + col];
            }
            accC[row * size + col] = sum;
        });
    });
}

void draw_line(int n, char ch) {
    for (int i = 0; i < n; ++i) {
        std::cout << ch;
    }
    std::cout << std::endl;
}

int main() {
    /* This doesn't work if SIZE > 832*/
    // You can figure out your limit using `ulimit -s` in my case 8192 is my limit
    // If SIZE = 833 then 
    // float A[SIZE * SIZE];
    // float B[SIZE * SIZE];
    // float C[SIZE * SIZE];
    /* Don't do the above but instead create heap memory */

    std::vector<float> A(SIZE * SIZE);
    std::vector<float> B(SIZE * SIZE);
    std::vector<float> C(SIZE * SIZE);

    // Initialize A and B matrices here...
    for (int i = 0; i < SIZE; i++) {
        for (int j = 0; j < SIZE; j++) {
            A[i * SIZE + j] = static_cast<float>(i + j);
            B[i * SIZE + j] = static_cast<float>(i - j);
        }
    }
    matrix_multiply(A.data(), B.data(), C.data(), SIZE);
    
    // ASCII output for visualization
    draw_line(50, '-');
    std::cout << "Resultant Matrix: " << std::endl;
    draw_line(50, '-');

    for (int i = 0; i < SIZE; ++i) {
        for (int j = 0; j < SIZE; ++j) {
            // Display each element. Limit the display for readability.
            if (j < 10 && i < 10) {
                std::cout << std::setw(5) << C[i * SIZE + j] << " ";
            }
        }
        if (i < 10) {
            std::cout << std::endl;
        }
    }
    draw_line(50, '-');
    return 0;
}
