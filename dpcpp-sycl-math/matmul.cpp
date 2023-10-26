#include <CL/sycl.hpp>

const int SIZE = 1024;

void basic_sycl_math(cl::sycl::float4 a, cl::sycl::float4 b, cl::sycl::float4 c, int size) {
    cl::sycl::queue q = sycl::queue(sycl::gpu_selector_v);
    std::cout << "Running on "
                << q.get_device().get_info<cl::sycl::info::device::name>()
                << "\n";
    {
        cl::sycl::buffer<cl::sycl::float4, 1> a_sycl(&a, cl::sycl::range<1>(1));
        cl::sycl::buffer<cl::sycl::float4, 1> b_sycl(&b, cl::sycl::range<1>(1));
        cl::sycl::buffer<cl::sycl::float4, 1> c_sycl(&c, cl::sycl::range<1>(1));
    
        q.submit([&] (cl::sycl::handler& cgh) {
            auto a_acc = a_sycl.get_access<cl::sycl::access::mode::read>(cgh);
            auto b_acc = b_sycl.get_access<cl::sycl::access::mode::read>(cgh);
            auto c_acc = c_sycl.get_access<cl::sycl::access::mode::discard_write>(cgh);

            cgh.single_task<class vector_addition>([=] () {
            c_acc[0] = a_acc[0] + b_acc[0];
            });
        });
    }
    std::cout << "  A { " << a.x() << ", " << a.y() << ", " << a.z() << ", " << a.w() << " }\n"
            << "+ B { " << b.x() << ", " << b.y() << ", " << b.z() << ", " << b.w() << " }\n"
            << "------------------\n"
            << "= C { " << c.x() << ", " << c.y() << ", " << c.z() << ", " << c.w() << " }"
            << std::endl;
}

int main() {
    cl::sycl::float4 a = { 1.0, 2.0, 3.0, 4.0 };
    cl::sycl::float4 b = { 4.0, 3.0, 2.0, 1.0 };
    cl::sycl::float4 c = { 0.0, 0.0, 0.0, 0.0 };

    // basic_sycl_math(A, B, C, SIZE);
    basic_sycl_math(a, b, c, SIZE);

    // Print or validate the results here, if necessary...

    return 0;
}
