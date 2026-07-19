#include "platform/wayland/window/WaylandWindow.hxx"

#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <wayland-client-protocol.h>

#include <cstdint>
#include <cstring>
#include <unordered_set>

#include "platform/wayland/cursor/WaylandCursor.hxx"
#include "platform/wayland/desktop/WaylandDecoration.hxx"
#include "platform/wayland/input/WaylandJoystick.hxx"
#include "platform/wayland/monitor/WaylandMonitor.hxx"

static int createAnonymousFile(off_t size) {
    int fd = memfd_create("vera-shm", MFD_CLOEXEC);
    if (fd < 0) return -1;
    if (ftruncate(fd, size) < 0) {
        close(fd);
        return -1;
    }
    return fd;
}

struct WaylandBuffer {
    wl_buffer* wlBuffer = nullptr;
    void* data = nullptr;
    size_t size = 0;
};

static WaylandBuffer createFallbackBuffer(wl_shm* shm, int width, int height) {
    WaylandBuffer buffer;
    if (!shm || width <= 0 || height <= 0) return buffer;

    int stride = width * 4;
    buffer.size = stride * height;

    int fd = createAnonymousFile(buffer.size);
    if (fd < 0) return buffer;

    buffer.data =
        mmap(nullptr, buffer.size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (buffer.data == MAP_FAILED) {
        close(fd);
        return buffer;
    }

    std::memset(buffer.data, 0x1A, buffer.size);

    wl_shm_pool* pool = wl_shm_create_pool(shm, fd, buffer.size);
    buffer.wlBuffer = wl_shm_pool_create_buffer(pool, 0, width, height, stride,
                                                WL_SHM_FORMAT_XRGB8888);

    wl_shm_pool_destroy(pool);
    close(fd);

    return buffer;
}

// INFO: Left the comments to understand it later
// Internal tracking set to identify surfaces bound to a GPU API (Vulkan/EGL)
// This requires zero header file changes and safely isolates the race
// condition.
static std::unordered_set<wl_surface*> gGraphicsTaintedSurfaces;

static void xdgSurfaceHandleConfigure(void* data, xdg_surface* xdgSurface,
                                      uint32_t serial) {
    // INFO: Left the comments
    auto* window = static_cast<WaylandWindow*>(data);

    // Always acknowledge the configuration transaction immediately
    xdg_surface_ack_configure(xdgSurface, serial);

    // Check if this window has bound a graphics context or has had its native
    // handle requested by a GPU subsystem (like Vulkan WSI)
    bool isGpuSurface = window->hasGraphicsContext() ||
                        (gGraphicsTaintedSurfaces.find(window->surface()) !=
                         gGraphicsTaintedSurfaces.end());

    if (isGpuSurface) {
        // GPU Mode: Never attach SHM fallback buffers. A bare commit is 100%
        // valid and safely avoids explicit sync timeline errors (Error 4).
        if (window->hasActiveGraphicsBuffer()) {
            window->triggerResizeCallback();
        } else {
            wl_surface_commit(window->surface());
        }
    } else {
        // Pure CPU/Software Path: Explicit sync is not engaged on this surface,
        // making it completely safe to attach and commit an SHM fallback
        // buffer.
        WaylandBuffer fallback = createFallbackBuffer(
            window->context().shm, window->width(), window->height());
        if (fallback.wlBuffer) {
            wl_surface_attach(window->surface(), fallback.wlBuffer, 0, 0);
            wl_surface_damage(window->surface(), 0, 0, window->width(),
                              window->height());
            window->setFallbackBuffer(fallback.wlBuffer);
            wl_surface_commit(window->surface());
        }
    }
}

static const xdg_surface_listener KXDG_SURFACE_LISTENER = {
    .configure = xdgSurfaceHandleConfigure};

static void xdgToplevelHandleConfigure(void* data, xdg_toplevel* xdgToplevel,
                                       int32_t width, int32_t height,
                                       wl_array* states) {
    auto* window = static_cast<WaylandWindow*>(data);
    (void)xdgToplevel;
    window->handleConfigure(width, height, states);
}

static void xdgToplevelHandleClose(void* data, xdg_toplevel* xdgToplevel) {
    auto* window = static_cast<WaylandWindow*>(data);
    (void)xdgToplevel;
    window->handleWmCloseRequest();
}

static const xdg_toplevel_listener KXDG_TOP_LEVEL_LISTENER = {
    .configure = xdgToplevelHandleConfigure,
    .close = xdgToplevelHandleClose,
    .configure_bounds = nullptr,
    .wm_capabilities = nullptr};

WaylandWindow::WaylandWindow(WaylandContext& ctx, VeraWindowHandle handle,
                             const VeraWindowInfo& info)
    : m_ctx(ctx),
      m_handle(handle),
      m_isResizing(false),
      m_pendingDeletion(false),
      m_hasGraphicsContext(false) {
    m_width = info.width;
    m_height = info.height;
    initSurface(info);
}

WaylandWindow::~WaylandWindow() { destroySurface(); }

void WaylandWindow::initSurface(const VeraWindowInfo& info) {
    if (!m_ctx.compositor || !m_ctx.wmBase) return;
    m_isResizing = false;

    m_surface = wl_compositor_create_surface(m_ctx.compositor);
    if (!m_surface) return;

    // Defensively resolve the monitor scale factor
    auto monitorInfo = getPrimaryMonitorWayland(m_ctx);
    int32_t scale = monitorInfo.dpiScale;

    // STRICT GUARD: Wayland protocol will instantly crash if scale < 1
    if (scale < 1) {
        scale = 1;
    }
    wl_surface_set_buffer_scale(m_surface, scale);

    m_ctx.windowsBySurface[m_surface] = this;

    m_xdgSurface = xdg_wm_base_get_xdg_surface(m_ctx.wmBase, m_surface);
    if (!m_xdgSurface) return;  // Defensive early exit
    xdg_surface_add_listener(m_xdgSurface, &KXDG_SURFACE_LISTENER, this);

    m_xdgToplevel = xdg_surface_get_toplevel(m_xdgSurface);
    if (!m_xdgToplevel) return;  // Defensive early exit
    xdg_toplevel_add_listener(m_xdgToplevel, &KXDG_TOP_LEVEL_LISTENER, this);

    xdg_toplevel_set_title(m_xdgToplevel, info.title.c_str());
    xdg_toplevel_set_app_id(m_xdgToplevel, "VeraEngine");

    m_decoration = createDecorationWayland(m_ctx, m_xdgToplevel);

    wl_surface_commit(m_surface);
}

void WaylandWindow::destroySurface() {
    m_keyCallback = {};
    m_mouseButtonCallback = {};
    m_mouseMoveCallback = {};
    m_scrollCallback = {};
    m_charCallback = {};

    if (m_ctx.focusedWindow == this) {
        m_ctx.focusedWindow = nullptr;
        m_ctx.keyboardFocusSurface = nullptr;
    }

    if (m_ctx.pointerFocusSurface == m_surface) {
        m_ctx.pointerFocusSurface = nullptr;
    }

    if (m_ctx.pressedKeys.size()) {
        m_ctx.pressedKeys.clear();
    }

    if (m_fallbackBuffer) {
        wl_buffer_destroy(m_fallbackBuffer);
        m_fallbackBuffer = nullptr;
    }

    if (m_surface) {
        gGraphicsTaintedSurfaces.erase(m_surface);
        m_ctx.windowsBySurface.erase(m_surface);
    }

    if (m_decoration) {
        destroyDecorationWayland(m_decoration);
        m_decoration = nullptr;
    }

    if (m_xdgToplevel) {
        xdg_toplevel_destroy(m_xdgToplevel);
        m_xdgToplevel = nullptr;
    }

    if (m_xdgSurface) {
        xdg_surface_destroy(m_xdgSurface);
        m_xdgSurface = nullptr;
    }

    if (m_surface) {
        wl_surface_destroy(m_surface);
        m_surface = nullptr;
    }
}

VeraWindowHandle WaylandWindow::getHandle() const { return m_handle; }

VeraNativeHandle WaylandWindow::getNativeHandle() const {
    VeraNativeHandle handle{};
    handle.display = m_ctx.display;
    handle.waylandSurface = m_surface;

    // As soon as a graphics context initialization requests the raw surface
    // handle, we flag it so the configuration callback knows explicit sync
    // rules apply.
    if (m_surface) {
        gGraphicsTaintedSurfaces.insert(m_surface);
    }
    return handle;
}

void WaylandWindow::setSize(uint32_t width, uint32_t height) {
    m_width = width;
    m_height = height;
    if (m_resizeCallback) m_resizeCallback(width, height);
}

void WaylandWindow::setPosition(int32_t x, int32_t y) {
    (void)x;
    (void)y;
}

void WaylandWindow::setMinSize(uint32_t width, uint32_t height) {
    if (m_xdgToplevel) {
        xdg_toplevel_set_min_size(m_xdgToplevel, static_cast<int32_t>(width),
                                  static_cast<int32_t>(height));
        if (!m_hasGraphicsContext) {
            wl_surface_commit(m_surface);
        }
    }
}

void WaylandWindow::setMaxSize(uint32_t width, uint32_t height) {
    if (m_xdgToplevel) {
        xdg_toplevel_set_max_size(m_xdgToplevel, static_cast<int32_t>(width),
                                  static_cast<int32_t>(height));
        if (!m_hasGraphicsContext) {
            wl_surface_commit(m_surface);
        }
    }
}

VeraWindowState WaylandWindow::getState() const { return m_state; }

void WaylandWindow::show() {
    if (!m_hasGraphicsContext) {
        wl_surface_commit(m_surface);
    }
}

void WaylandWindow::hide() { destroySurface(); }

void WaylandWindow::minimize() {
    if (m_xdgToplevel) {
        xdg_toplevel_set_minimized(m_xdgToplevel);
    }
}

void WaylandWindow::maximize() {
    if (m_xdgToplevel) {
        xdg_toplevel_set_maximized(m_xdgToplevel);
    }
}

void WaylandWindow::restore() {
    if (m_xdgToplevel) {
        xdg_toplevel_unset_maximized(m_xdgToplevel);
    }
}

void WaylandWindow::close() { handleWmCloseRequest(); }

void WaylandWindow::handleWmCloseRequest() {
    if (m_closeRequestCallback) {
        if (!m_closeRequestCallback()) return;
    }

    m_pendingDeletion = true;
}

void WaylandWindow::focus() {}

void WaylandWindow::setTitle(const std::string& title) {
    if (m_xdgToplevel) {
        xdg_toplevel_set_title(m_xdgToplevel, title.c_str());
    }
}

void WaylandWindow::setFullscreen(FullScreenMode mode) {
    if (!m_xdgToplevel) return;

    if (mode != FullScreenMode::Windowed) {
        xdg_toplevel_set_fullscreen(m_xdgToplevel, nullptr);
    } else {
        xdg_toplevel_unset_fullscreen(m_xdgToplevel);
    }

    if (!m_hasGraphicsContext) {
        wl_surface_commit(m_surface);
    }
}

void WaylandWindow::setAlwaysOnTop(bool value) { (void)value; }

void WaylandWindow::setIcon(const std::string& iconPath) { (void)iconPath; }

void WaylandWindow::setTitlebarHitTestRegions(
    const VeraHitTestRegions& regions) {
    m_hitTestRegions = regions;
}

void WaylandWindow::setResizeCallback(
    std::function<void(uint32_t, uint32_t)> callback) {
    m_resizeCallback = callback;
}
void WaylandWindow::setMoveCallback(
    std::function<void(int32_t, int32_t)> callback) {
    m_moveCallback = callback;
}
void WaylandWindow::setCloseRequestCallback(std::function<bool()> callback) {
    m_closeRequestCallback = callback;
}
void WaylandWindow::setFocusChangeCallback(std::function<void(bool)> callback) {
    m_focusChangeCallback = callback;
}
void WaylandWindow::setDpiChangeCallback(std::function<void(float)> callback) {
    m_dpiChangeCallback = callback;
}
void WaylandWindow::setKeyCallback(
    std::function<void(VeraKey, bool, bool)> callback) {
    m_keyCallback = callback;
}
void WaylandWindow::setMouseButtonCallback(
    std::function<void(VeraMouseButton, bool)> callback) {
    m_mouseButtonCallback = callback;
}
void WaylandWindow::setMouseMoveCallback(
    std::function<void(double, double)> callback) {
    m_mouseMoveCallback = callback;
}
void WaylandWindow::setScrollCallback(
    std::function<void(double, double)> callback) {
    m_scrollCallback = callback;
}
void WaylandWindow::setCharCallback(std::function<void(uint32_t)> callback) {
    m_charCallback = callback;
}

void WaylandWindow::setCursorMode(VeraCursorMode mode) {
    setCursorModeWayland(m_ctx, m_surface, mode);
}

void WaylandWindow::setCursorShape(VeraCursorShape shape) {
    setCursorShapeWayland(m_ctx, shape);
}

VeraMonitorInfo WaylandWindow::getCurrentMonitor() const {
    return getPrimaryMonitorWayland(m_ctx);
}

void WaylandWindow::handleConfigure(int32_t width, int32_t height,
                                    wl_array* states) {
    m_isResizing = false;

    if (states && states->size > 0) {
        const auto* dataBegin = static_cast<const uint32_t*>(states->data);
        const size_t numElements = states->size / sizeof(uint32_t);

        for (size_t i = 0; i < numElements; ++i) {
            if (dataBegin[i] == XDG_TOPLEVEL_STATE_RESIZING) {
                m_isResizing = true;
                break;
            }
        }
    }

    if (width > 0 && height > 0) {
        m_state.width = static_cast<uint32_t>(width);
        m_state.height = static_cast<uint32_t>(height);

        if (m_resizeCallback) {
            m_resizeCallback(m_state.width, m_state.height);
        }
    }
}

void WaylandWindow::triggerResizeCallback() {
    if (m_resizeCallback) {
        m_resizeCallback(m_state.width, m_state.height);
    }
}

void WaylandWindow::markGraphicsContextActive() {
    m_hasGraphicsContext = true;
    if (m_fallbackBuffer) {
        wl_buffer_destroy(m_fallbackBuffer);
        m_fallbackBuffer = nullptr;
    }
}

void WaylandWindow::setFallbackBuffer(wl_buffer* buffer) {
    if (m_fallbackBuffer) {
        wl_buffer_destroy(m_fallbackBuffer);
    }
    m_fallbackBuffer = buffer;
}

void WaylandWindow::setJoystickButtonCallback(
    VeraJoystickButtonCallback callback) {
    m_joyButtonCallback = callback;
    setJoystickButtonCallbackWayland(
        [this](uint32_t id, uint32_t btn, bool pressed) {
            if (m_joyButtonCallback) m_joyButtonCallback(id, btn, pressed);
        });
}

void WaylandWindow::setJoystickAxisCallback(VeraJoystickAxisCallback callback) {
    m_joyAxisCallback = callback;
    setJoystickAxisCallbackWayland(
        [this](uint32_t id, uint32_t axis, float val) {
            if (m_joyAxisCallback) m_joyAxisCallback(id, axis, val);
        });
}

void WaylandWindow::setDestructionCallback(
    std::function<void(VeraWindow*)> callback) {
    (void)callback;
}

// Map platform-specific enum entries to sequential linux joydev button numbers
static constexpr uint8_t mapToLinuxJoydevButton(VeraJoystickButton button) {
    switch (button) {
        // Face Buttons
        case VeraJoystickButton::Cross:
        case VeraJoystickButton::XboxA:
            return 0;
        case VeraJoystickButton::Circle:
        case VeraJoystickButton::XboxB:
            return 1;
        case VeraJoystickButton::Square:
        case VeraJoystickButton::XboxX:
            return 2;
        case VeraJoystickButton::Triangle:
        case VeraJoystickButton::XboxY:
            return 3;

        // Bumpers
        case VeraJoystickButton::L1:
        case VeraJoystickButton::XboxLB:
            return 4;
        case VeraJoystickButton::R1:
        case VeraJoystickButton::XboxRB:
            return 5;

        // Triggers (Digital/Click Emulation)
        case VeraJoystickButton::L2:
        case VeraJoystickButton::XboxLT:
            return 6;
        case VeraJoystickButton::R2:
        case VeraJoystickButton::XboxRT:
            return 7;

        // Menu / System Options
        case VeraJoystickButton::Share:
        case VeraJoystickButton::XboxBack:
            return 8;
        case VeraJoystickButton::Options:
        case VeraJoystickButton::XboxStart:
            return 9;
        case VeraJoystickButton::PS:
        case VeraJoystickButton::XboxGuide:
            return 10;

        // Stick Clicks
        case VeraJoystickButton::L3:
        case VeraJoystickButton::XboxLS:
            return 11;
        case VeraJoystickButton::R3:
        case VeraJoystickButton::XboxRS:
            return 12;

        // Directional D-Pad Fallbacks
        case VeraJoystickButton::DpadUp:
            return 13;
        case VeraJoystickButton::DpadDown:
            return 14;
        case VeraJoystickButton::DpadLeft:
            return 15;
        case VeraJoystickButton::DpadRight:
            return 16;

        // Platform Exclusives
        case VeraJoystickButton::Touchpad:
            return 17;
        case VeraJoystickButton::XboxShare:
            return 18;

        default:
            return 255;
    }
}

bool WaylandWindow::isPressed(VeraPressable input) const {
    // Rule: If this specific window does not have active system focus,
    // it shouldn't respond to any input states.
    if (!this->m_state.isFocused) {
        return false;
    }

    return std::visit(
        [this](auto&& arg) -> bool {
            using T = std::decay_t<decltype(arg)>;

            // 1. Keyboard State (Read from central context, filtered by window
            // focus)
            if constexpr (std::is_same_v<T, VeraKey>) {
                if (arg == VeraKey::Unknown || arg == VeraKey::Count) {
                    return false;
                }
                size_t keyIndex = static_cast<size_t>(arg);
                if (keyIndex < std::size(this->m_ctx.keyStates)) {
                    return this->m_ctx.keyStates[keyIndex];
                }
                return false;
            }

            // 2. Mouse Button State (Read from central context, filtered by
            // window focus)
            else if constexpr (std::is_same_v<T, VeraMouseButton>) {
                if (arg == VeraMouseButton::Count) return false;
                size_t mouseIndex = static_cast<size_t>(arg);
                if (mouseIndex < std::size(this->m_ctx.mouseButtonStates)) {
                    return this->m_ctx.mouseButtonStates[mouseIndex];
                }
                return false;
            }

            // 3. Gamepad Evaluation (Read from central context, filtered by
            // window focus)
            else if constexpr (std::is_same_v<T, VeraJoystickButton>) {
                if (arg == VeraJoystickButton::Count) return false;
                uint8_t rawButtonId = mapToLinuxJoydevButton(arg);
                if (rawButtonId == 255) return false;

                // Correctly pass m_ctx as the first argument matching your
                // function contract
                VeraJoystickState joyState =
                    getStateJoystickWayland(this->m_ctx, 0);
                if (joyState.connected &&
                    rawButtonId < joyState.buttons.size()) {
                    return joyState.buttons[rawButtonId];
                }
                return false;
            }

            return false;
        },
        input);
}
