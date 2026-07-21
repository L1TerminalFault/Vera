#include "BackendFactory.h"

#if defined(__linux__)
#include "../platform/linux/LinuxBackend.h"
#elif defined(_WIN32)
#include "../platform/win32/Win32Backend.h"
#endif

std::unique_ptr<IPlatformBackend> makePlatformBackend() {
#if defined(__linux__)

    return std::make_unique<LinuxBackend>();

#elif defined(_WIN32)

    return std::make_unique<Win32Backend>();

#else

    return nullptr;

#endif
}
