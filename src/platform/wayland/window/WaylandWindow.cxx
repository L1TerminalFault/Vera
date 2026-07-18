#include "platform/wayland/window/WaylandWindow.hxx"

#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

#include <cstring>

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

static void xdgSurfaceHandleConfigure(void* data, xdg_surface* xdgSurface,
                                      uint32_t serial) {
    auto* window = static_cast<WaylandWindow*>(data);
    xdg_surface_ack_configure(xdgSurface, serial);

    if (!window->hasActiveGraphicsBuffer()) {
        WaylandBuffer fallback = createFallbackBuffer(
            window->context().shm, window->width(), window->height());

        if (fallback.wlBuffer) {
            wl_surface_attach(window->surface(), fallback.wlBuffer, 0, 0);
            wl_surface_damage(window->surface(), 0, 0, window->width(),
                              window->height());
            window->setFallbackBuffer(fallback.wlBuffer);
        }

        wl_surface_commit(window->surface());
    } else {
        wl_surface_commit(window->surface());

        window->triggerResizeCallback();
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
    : m_ctx(ctx), m_handle(handle) {
    m_width = info.width;
    m_height = info.height;
    initSurface(info);
}

WaylandWindow::~WaylandWindow() { destroySurface(); }

void WaylandWindow::initSurface(const VeraWindowInfo& info) {
    if (!m_ctx.compositor || !m_ctx.wmBase) return;

    m_surface = wl_compositor_create_surface(m_ctx.compositor);
    if (!m_surface) return;

    m_ctx.windowsBySurface[m_surface] = this;

    m_xdgSurface = xdg_wm_base_get_xdg_surface(m_ctx.wmBase, m_surface);
    xdg_surface_add_listener(m_xdgSurface, &KXDG_SURFACE_LISTENER, this);

    m_xdgToplevel = xdg_surface_get_toplevel(m_xdgSurface);
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
        wl_surface_commit(m_surface);
    }
}

void WaylandWindow::setMaxSize(uint32_t width, uint32_t height) {
    if (m_xdgToplevel) {
        xdg_toplevel_set_max_size(m_xdgToplevel, static_cast<int32_t>(width),
                                  static_cast<int32_t>(height));
        wl_surface_commit(m_surface);
    }
}

VeraWindowState WaylandWindow::getState() const { return m_state; }

void WaylandWindow::show() { wl_surface_commit(m_surface); }

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
    wl_surface_commit(m_surface);
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
