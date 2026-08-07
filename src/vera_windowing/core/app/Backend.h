#pragma once

#if defined(_WIN32)
#include "platform/win32/Win32Backend.h"
#else
#include "platform/wayland/WaylandBackend.hxx"
#include "platform/x11/X11Backend.hxx"
#endif

nostd::UniquePtr<IBackend> create(const VeraAppInfo& info) {
#if defined(_WIN32)
    (void)info;
    return std::make_unique<Win32Backend>();
#else
    VeraLinuxProtocol protocol = info.preferedLinuxProtocol;

    if (protocol == VeraLinuxProtocol::Auto) {
        const char* waylandDisplay = std::getenv("WAYLAND_DISPLAY");
        protocol = (waylandDisplay && waylandDisplay[0] != '\0')
                       ? VeraLinuxProtocol::Wayland
                       : VeraLinuxProtocol::X11;
    }

    if (protocol == VeraLinuxProtocol::Wayland) {
        auto backend = nostd::makeUnique<WaylandBackend>();
        if (backend->initialize(info)) {
            return nostd::UniquePtr<IBackend>(backend.release());
        }
    }

    auto x11 = nostd::makeUnique<X11Backend>();
    if (x11->initialize(info)) {
        return nostd::UniquePtr<IBackend>(x11.release());
    }
    return nullptr;
#endif
}
