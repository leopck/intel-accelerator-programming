#include <level_zero/ze_api.h>
#include <iostream>


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

    std::cout << "Running createContext" << std::endl;
    ze_context_handle_t contextHandle = createContext(driverHandle);
    if (!contextHandle) {
        std::cerr << "Failed to create the Level Zero context!" << std::endl;
        return -1;
    }

}