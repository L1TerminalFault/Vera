#include <cstdlib>

#if defined(_WIN32)
#include "platform/win32/Win32Backend.h"
#else
#include "platform/wayland/WaylandBackend.hxx"
#include "platform/x11/X11Backend.hxx"
#endif

namespace vera::core::platform {

std::unique_ptr<IPlatformBackend> create(const VeraAppInfo& info) {
#if defined(_WIN32)
    (void)info;
    return std::make_unique<win32::Win32Backend>();
#else
    VeraLinuxProtocol protocol = info.preferedLinuxProtocol;

    if (protocol == VeraLinuxProtocol::Auto) {
        const char* waylandDisplay = std::getenv("WAYLAND_DISPLAY");
        protocol = (waylandDisplay && waylandDisplay[0] != '\0')
                       ? VeraLinuxProtocol::Wayland
                       : VeraLinuxProtocol::X11;
    }

    if (protocol == VeraLinuxProtocol::Wayland) {
        auto backend = std::make_unique<vera::wayland::WaylandBackend>();
        if (backend->initialize(info)) {
            return backend;
        }
        // Wayland compositor not reachable -- fall back to X11 (XWayland or
        // a genuine X server) rather than hard-failing.
    }

    auto x11 = std::make_unique<vera::x11::X11Backend>();
    if (x11->initialize(info)) {
        return x11;
    }
    return nullptr;
#endif
}

}  // namespace vera::core::platform
