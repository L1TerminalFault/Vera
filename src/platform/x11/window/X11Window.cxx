#include "platform/x11/window/X11Window.hxx"

#include <X11/Xatom.h>

#include <cstdint>

#include "platform/x11/monitor/X11Monitor.hxx"
#include "platform/x11/window/X11Fullscreen.hxx"

#undef Status
#undef Bool

#include "platform/x11/cursor/X11Cursor.hxx"
#include "platform/x11/input/X11Joystick.hxx"
#include "platform/x11/input/X11Mouse.hxx"
#include "platform/x11/window/X11Decoration.hxx"
#include "platform/x11/window/X11Properties.hxx"

X11Window::X11Window(X11Context& ctx, Window xid, VeraWindowHandle handle,
                     const VeraWindowInfo& info)
    : m_ctx(ctx),
      m_xid(xid),
      m_handle(handle),
      m_customTitleBar(info.customTitleBar) {
    m_state.width = info.width;
    m_state.height = info.height;
    m_state.x = info.x.value_or(0);
    m_state.y = info.y.value_or(0);
    m_state.isVisible = info.startVisible;

    ctx.windowsByXid[xid] = this;

    XSelectInput(ctx.display, xid,
                 ExposureMask | StructureNotifyMask | FocusChangeMask |
                     KeyPressMask | KeyReleaseMask | ButtonPressMask |
                     ButtonReleaseMask | PointerMotionMask |
                     PropertyChangeMask);

    XSetWMProtocols(ctx.display, xid, &ctx.atoms.wmDeleteWindow, 1);

    // Create the window-specific Input Context (XIC)
    if (ctx.xim) {
        m_xic = XCreateIC(ctx.xim, XNInputStyle,
                          XIMPreeditNothing | XIMStatusNothing, XNClientWindow,
                          m_xid, XNFocusWindow, m_xid, nullptr);
    }

    setTitleX11(ctx, xid, info.title);
    setSizeHintsX11(ctx, xid, info.minWidth, info.minHeight, info.maxWidth,
                    info.maxHeight, info.resizable);
    setWindowTypeX11(ctx, xid);
    setPidX11(ctx, xid);
    if (!info.decorated) setDecoratedX11(ctx, xid, false);
    if (info.alwaysOnTop) setAlwaysOnTopX11(ctx, xid, true);
    if (!info.iconPath.empty()) setIconX11(ctx, xid, info.iconPath);

    // XDND: advertise that we accept drops (version 5).
    Atom xdndVersion = 5;
    XChangeProperty(ctx.display, xid, ctx.atoms.xdndAware, XA_ATOM, 32,
                    PropModeReplace,
                    reinterpret_cast<unsigned char*>(&xdndVersion), 1);

    if (info.startVisible) show();
    if (info.startMaximized) maximize();
    if (info.startMinimized) minimize();
    if (info.fullscreenMode != FullScreenMode::Windowed) {
        setFullscreen(info.fullscreenMode);
    }
}

X11Window::~X11Window() {
    if (m_xic) {
        XDestroyIC(m_xic);
        m_xic = nullptr;
    }
    m_ctx.windowsByXid.erase(m_xid);
    XDestroyWindow(m_ctx.display, m_xid);
}

VeraWindowHandle X11Window::getHandle() const { return m_handle; }

VeraNativeHandle X11Window::getNativeHandle() const {
    VeraNativeHandle handle;
    handle.display = m_ctx.display;
    handle.x11Window = static_cast<uint64_t>(m_xid);
    return handle;
}

void X11Window::setSize(uint32_t width, uint32_t height) {
    XResizeWindow(m_ctx.display, m_xid, width, height);
}
void X11Window::setPosition(int32_t x, int32_t y) {
    XMoveWindow(m_ctx.display, m_xid, x, y);
}
void X11Window::setMinSize(uint32_t width, uint32_t height) {
    setSizeHintsX11(m_ctx, m_xid, width, height, 0, 0, true);
}
void X11Window::setMaxSize(uint32_t width, uint32_t height) {
    setSizeHintsX11(m_ctx, m_xid, 0, 0, width, height, true);
}
VeraWindowState X11Window::getState() const { return m_state; }

void X11Window::show() { XMapWindow(m_ctx.display, m_xid); }
void X11Window::hide() { XUnmapWindow(m_ctx.display, m_xid); }
void X11Window::minimize() {
    XIconifyWindow(m_ctx.display, m_xid, m_ctx.screen);
}
void X11Window::maximize() {
    setNetWmStateX11(m_ctx, m_xid, m_ctx.atoms.netWmStateMaximizedHorz,
                     m_ctx.atoms.netWmStateMaximizedVert, true);
}
void X11Window::restore() {
    setNetWmStateX11(m_ctx, m_xid, m_ctx.atoms.netWmStateMaximizedHorz,
                     m_ctx.atoms.netWmStateMaximizedVert, false);
    XMapWindow(m_ctx.display, m_xid);
}

void X11Window::close() {
    if (!m_closeRequestCallback || m_closeRequestCallback()) {
        XEvent event{};
        event.type = ClientMessage;
        event.xclient.window = m_xid;
        event.xclient.message_type = m_ctx.atoms.wmProtocols;
        event.xclient.format = 32;
        event.xclient.data.l[0] =
            static_cast<int64_t>(m_ctx.atoms.wmDeleteWindow);
        XSendEvent(m_ctx.display, m_xid, False, NoEventMask, &event);
    }
}

void X11Window::handleWmCloseRequest() {
    if (m_closeRequestCallback) {
        if (!m_closeRequestCallback()) return;
    }

    m_pendingDeletion = true;
}

void X11Window::focus() {
    XSetInputFocus(m_ctx.display, m_xid, RevertToParent, CurrentTime);
    XRaiseWindow(m_ctx.display, m_xid);
}
void X11Window::setTitle(const std::string& title) {
    setTitleX11(m_ctx, m_xid, title);
}
void X11Window::setFullscreen(FullScreenMode mode) {
    applyFullscreenX11(m_ctx, m_xid, mode);
    m_state.isFullscreen = (mode != FullScreenMode::Windowed);
}
void X11Window::setAlwaysOnTop(bool value) {
    setAlwaysOnTopX11(m_ctx, m_xid, value);
}
void X11Window::setIcon(const std::string& iconPath) {
    setIconX11(m_ctx, m_xid, iconPath);
}

void X11Window::setTitlebarHitTestRegions(const VeraHitTestRegions& regions) {
    m_hitTestRegions = regions;
}

void X11Window::setResizeCallback(
    std::function<void(uint32_t, uint32_t)> callback) {
    m_resizeCallback = std::move(callback);
}
void X11Window::setMoveCallback(
    std::function<void(int32_t, int32_t)> callback) {
    m_moveCallback = std::move(callback);
}
void X11Window::setCloseRequestCallback(std::function<bool()> callback) {
    m_closeRequestCallback = std::move(callback);
}
void X11Window::setFocusChangeCallback(std::function<void(bool)> callback) {
    m_focusChangeCallback = std::move(callback);
}
void X11Window::setDpiChangeCallback(std::function<void(float)> callback) {
    m_dpiChangeCallback = std::move(callback);
}

void X11Window::setKeyCallback(
    std::function<void(VeraKey, bool, bool)> callback) {
    m_keyCallback = std::move(callback);
}
void X11Window::setMouseButtonCallback(
    std::function<void(VeraMouseButton, bool)> callback) {
    m_mouseButtonCallback = std::move(callback);
}
void X11Window::setMouseMoveCallback(
    std::function<void(double, double)> callback) {
    m_mouseMoveCallback = std::move(callback);
}
void X11Window::setScrollCallback(
    std::function<void(double, double)> callback) {
    m_scrollCallback = std::move(callback);
}
void X11Window::setCharCallback(std::function<void(uint32_t)> callback) {
    m_charCallback = std::move(callback);
}

void X11Window::setCursorMode(VeraCursorMode mode) {
    m_cursorDisabled = (mode == VeraCursorMode::Disabled);
    applyCursorModeX11(m_ctx, m_xid, mode);
}
void X11Window::setCursorShape(VeraCursorShape shape) {
    applyCursorShapeX11(m_ctx, m_xid, shape);
}

VeraMonitorInfo X11Window::getCurrentMonitor() const {
    int32_t centerX = m_state.x + static_cast<int32_t>(m_state.width) / 2;
    int32_t centerY = m_state.y + static_cast<int32_t>(m_state.height) / 2;
    return getMonitorAtCoordinateXYX11(m_ctx, centerX, centerY);
}

void X11Window::setJoystickButtonCallback(VeraJoystickButtonCallback callback) {
    m_joyButtonCallback = callback;
    setJoystickButtonCallbackX11(
        [this](uint32_t id, uint32_t btn, bool pressed) {
            if (m_joyButtonCallback) m_joyButtonCallback(id, btn, pressed);
        });
}

void X11Window::setJoystickAxisCallback(VeraJoystickAxisCallback callback) {
    m_joyAxisCallback = callback;
    setJoystickAxisCallbackX11([this](uint32_t id, uint32_t axis, float val) {
        if (m_joyAxisCallback) m_joyAxisCallback(id, axis, val);
    });
}

void X11Window::handleXEvent(XEvent& event) {
    switch (event.type) {
        case ConfigureNotify: {
            auto& xc = event.xconfigure;
            if (static_cast<uint32_t>(xc.width) != m_state.width ||
                static_cast<uint32_t>(xc.height) != m_state.height) {
                m_state.width = xc.width;
                m_state.height = xc.height;
                if (m_resizeCallback) {
                    m_resizeCallback(m_state.width, m_state.height);
                }
            }
            if (xc.x != m_state.x || xc.y != m_state.y) {
                m_state.x = xc.x;
                m_state.y = xc.y;
                if (m_moveCallback) m_moveCallback(m_state.x, m_state.y);
            }
            break;
        }
        case MapNotify:
            m_state.isVisible = true;
            m_state.isMinimized = false;
            break;
        case UnmapNotify:
            m_state.isVisible = false;
            break;
        case FocusIn:
            m_state.isFocused = true;
            if (m_xic) XSetICFocus(m_xic);  // Set layout focus
            if (m_focusChangeCallback) m_focusChangeCallback(true);
            break;
        case FocusOut:
            m_state.isFocused = false;
            if (m_xic) XUnsetICFocus(m_xic);  // Unset layout focus
            if (m_focusChangeCallback) m_focusChangeCallback(false);
            break;
        case KeyPress:
            handleKeyPressX11(m_ctx, event.xkey, m_keyState, m_keyCallback,
                              m_charCallback);
            break;
        case KeyRelease:
            handleKeyReleaseX11(m_ctx, event.xkey, m_keyState, m_keyCallback);
            break;
        case ButtonPress:
            if (m_customTitleBar) {
                handleTitlebarButtonPressX11(m_ctx, m_xid, m_hitTestRegions,
                                             event.xbutton.x, event.xbutton.y);
            }
            handleMouseButtonPressX11(event.xbutton, m_mouseButtonCallback,
                                      m_scrollCallback);
            break;
        case ButtonRelease:
            handleMouseButtonReleaseX11(event.xbutton, m_mouseButtonCallback);
            break;
        case MotionNotify:
            if (m_cursorDisabled) {
                int centerX = static_cast<int>(m_state.width) / 2;
                int centerY = static_cast<int>(m_state.height) / 2;
                double dx = event.xmotion.x - centerX;
                double dy = event.xmotion.y - centerY;
                if ((dx != 0 || dy != 0) && m_mouseMoveCallback) {
                    m_mouseMoveCallback(dx, dy);
                }
                XWarpPointer(m_ctx.display, None, m_xid, 0, 0, 0, 0, centerX,
                             centerY);
            } else if (m_mouseMoveCallback) {
                m_mouseMoveCallback(event.xmotion.x, event.xmotion.y);
            }
            break;
        case PropertyNotify:
            if (event.xproperty.atom == m_ctx.atoms.netWmState) {
                m_state.isMaximized = hasNetWmStateX11(
                    m_ctx, m_xid, m_ctx.atoms.netWmStateMaximizedHorz);
                m_state.isFullscreen = hasNetWmStateX11(
                    m_ctx, m_xid, m_ctx.atoms.netWmStateFullscreen);
            }
            break;
        default:
            break;
    }
}
