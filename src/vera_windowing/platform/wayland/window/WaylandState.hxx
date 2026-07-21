#pragma once

#include <wayland-client.h>

#include "core/app/Types.h"
#include "platform/wayland/internal/protocols/xdg-shell-client-protocol.h"

VeraWindowState parseStates(wl_array* states, VeraWindowState current) {
    VeraWindowState updated = current;

    updated.isFocused = false;

    if (states && states->size > 0) {
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
                default:
                    break;
            }
        }
    }

    return updated;
}
