#include <optional>
#include <vector>
#include <string>
#include <iostream>
#include <cstring>
#include <fcntl.h>
#include <unistd.h>

#include <drm_fourcc.h> // For DRM_FORMAT_MOD_LINEAR

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/hwcontext.h>
#include <libavutil/hwcontext_vaapi.h>
#include <libavutil/pixdesc.h>
#include <va/va.h>
#include <va/va_drm.h>
#include <va/va_drmcommon.h>
}

int main() {
    std::cout << "Running drmFd" << std::endl;
    int drmFd = open("/dev/dri/renderD128", O_RDWR); // Opening the first render node. Change index as per your system.
    if (drmFd < 0) {
        // Handle error
        return -1;
    }

    std::cout << "Running vaGetDisplayDRM" << std::endl;
    VADisplay vaDisplay = vaGetDisplayDRM(drmFd);
    if (vaDisplay == nullptr) {
        // Handle error
        close(drmFd);
        return -1;
    }
    int major, minor;
    VAStatus vaStatus = vaInitialize(vaDisplay, &major, &minor);
    if (vaStatus != VA_STATUS_SUCCESS) {
        // Handle error
        vaTerminate(vaDisplay);
        close(drmFd);
        return -1;
    }

    // Cleanup
    std::cout << "Running vaTerminate" << std::endl;
    vaTerminate(vaDisplay);
    close(drmFd);
    return 0;
}