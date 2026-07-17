#pragma once

#include <wayland-client.h>
#include <wayland-cursor.h>
#include <xkbcommon/xkbcommon.h>

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

#include "platform/wayland/internal/protocols/pointer-constraints-unstable-v1-client-protocol.h"
#include "platform/wayland/internal/protocols/relative-pointer-unstable-v1-client-protocol.h"
#include "platform/wayland/internal/protocols/xdg-decoration-unstable-v1-client-protocol.h"
#include "platform/wayland/internal/protocols/xdg-output-unstable-v1-client-protocol.h"
#include "platform/wayland/internal/protocols/xdg-shell-client-protocol.h"

namespace vera::wayland::window {
class WaylandWindow;
}

namespace vera::wayland::internal {

using namespace window;

struct WaylandOutputInfo {
    wl_output* output = nullptr;
    uint32_t globalId = 0;
    std::string name;
    std::string model;
    int32_t x = 0;
    int32_t y = 0;
    int32_t widthMm = 0;
    int32_t heightMm = 0;
    int32_t scaleFactor = 1;
    int32_t refreshRate = 60000;
    bool isPrimary = false;
};

struct WaylandContext {
    wl_display* display = nullptr;
    wl_registry* registry = nullptr;
    wl_compositor* compositor = nullptr;
    wl_shm* shm = nullptr;
    wl_seat* seat = nullptr;
    xdg_wm_base* wmBase = nullptr;
    wl_data_device_manager* dataDeviceManager = nullptr;
    zxdg_output_v1* xdgOutputManager = nullptr;

    zxdg_decoration_manager_v1* decorationManager = nullptr;
    zwp_pointer_constraints_v1* pointerConstraints = nullptr;
    zwp_relative_pointer_manager_v1* relativePointerManager = nullptr;

    wl_pointer* pointer = nullptr;
    wl_keyboard* keyboard = nullptr;
    wl_data_device* dataDevice = nullptr;

    xkb_context* xkbContext = nullptr;
    xkb_keymap* xkbKeymap = nullptr;
    xkb_state* xkbState = nullptr;

    std::vector<WaylandOutputInfo> outputs;
    std::unordered_map<wl_surface*, WaylandWindow*> windowsBySurface;

    wl_surface* pointerFocusSurface = nullptr;
    wl_surface* keyboardFocusSurface = nullptr;
    WaylandWindow* focusedWindow = nullptr;

    uint32_t lastPointerEnterSerial = 0;
    uint32_t lastPointerButtonSerial = 0;
    uint32_t pointerSerial = 0;

    wl_data_offer* activeClipboardOffer = nullptr;
    wl_cursor_theme* cursorTheme = nullptr;
    wl_surface* cursorSurface = nullptr;

    double pointerX = 0.0;
    double pointerY = 0.0;
    bool quitRequested = false;

    zwp_locked_pointer_v1* activeLockedPointer = nullptr;
    zwp_confined_pointer_v1* activeConfinedPointer = nullptr;
    zwp_relative_pointer_v1* activeRelativePointer = nullptr;

    bool isFullscreen = false;
    bool isMaximized = false;
    bool isSuspended = false;

    uint64_t allocateHandle() {
        static uint64_t s_handleCounter = 0;
        return ++s_handleCounter;
    }
};

}  // namespace vera::wayland::internal
