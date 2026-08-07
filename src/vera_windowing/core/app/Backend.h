#pragma once

#include <cstdlib>

#include "Types.h"

#if defined(_WIN32)
#include "platform/win32/Win32Backend.h"
#elif defined(__linux__)
#if defined(VERA_HAS_WAYLAND)
#include "platform/wayland/WaylandBackend.hxx"
#endif
#if defined(VERA_HAS_X11)
#include "platform/x11/X11Backend.hxx"
#endif
#endif

nostd::UniquePtr<IBackend> create(const VeraAppInfo& info) {
#if defined(_WIN32)
    (void)info;
    return nostd::makeUnique<Win32Backend>();
#elif defined(__linux__)
    VeraLinuxProtocol protocol = info.preferedLinuxProtocol;

    if (protocol == VeraLinuxProtocol::Auto) {
        const char* waylandDisplay = std::getenv("WAYLAND_DISPLAY");
        protocol = (waylandDisplay && waylandDisplay[0] != '\0')
                       ? VeraLinuxProtocol::Wayland
                       : VeraLinuxProtocol::X11;
    }

#if defined(VERA_HAS_WAYLAND)
    if (protocol == VeraLinuxProtocol::Wayland) {
        auto backend = nostd::makeUnique<WaylandBackend>();
        if (backend->initialize(info)) {
            return nostd::UniquePtr<IBackend>(backend.release());
        }
    }
#endif

#if defined(VERA_HAS_X11)
    if (protocol == VeraLinuxProtocol::X11 ||
        protocol == VeraLinuxProtocol::Auto) {
        auto x11 = nostd::makeUnique<X11Backend>();
        if (x11->initialize(info)) {
            return nostd::UniquePtr<IBackend>(x11.release());
        }
    }
#endif

    return nullptr;
#else
    (void)info;
    return nullptr;
#endif
}
