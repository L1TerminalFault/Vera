#pragma once

#include <wayland-client.h>

#include "core/window/Window.h"
#include "platform/wayland/internal/protocols/xdg-shell-client-protocol.h"

namespace vera::wayland::window {
using namespace core::window;

VeraWindowState parseStates(wl_array* states, VeraWindowState current) {
    VeraWindowState updated = current;

    // Reset transient focus state; we'll re-evaluate it from the active list
    updated.isFocused = false;

    if (states && states->size > 0) {
        // Safe, explicit cast for C++ compatibility
        const auto* activeStates = static_cast<const uint32_t*>(states->data);
        size_t numStates = states->size / sizeof(uint32_t);

        for (size_t i = 0; i < numStates; ++i) {
            uint32_t state = activeStates[i];
            switch (state) {
                case XDG_TOPLEVEL_STATE_MAXIMIZED:
                    updated.isMaximized = true;
                    break;
                case XDG_TOPLEVEL_STATE_FULLSCREEN:
                    updated.isFullscreen = true;
                    break;
                case XDG_TOPLEVEL_STATE_ACTIVATED:
                    updated.isFocused = true;
                    break;
                // Note: XDG_TOPLEVEL_STATE_RESIZING and SUSPENDED are ignored
                // here because they are not part of the platform-agnostic
                // VeraWindowState struct.
                default:
                    break;
            }
        }
    }

    // Hand back the beautifully calculated state map
    return updated;
}

}  // namespace vera::wayland::window
