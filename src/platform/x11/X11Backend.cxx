#include "platform/x11/X11Backend.hxx"

#include <X11/Xlib.h>
#include <X11/Xlocale.h>

#include "platform/x11/events/X11Events.hxx"
#include "platform/x11/internal/X11XInput2.hxx"
#include "platform/x11/internal/X11XKB.hxx"
#include "platform/x11/monitor/X11Monitor.hxx"
#include "platform/x11/window/X11Window.hxx"

#undef Status

#include "platform/x11/cursor/X11Cursor.hxx"
#include "platform/x11/desktop/X11Clipboard.hxx"
#include "platform/x11/desktop/X11DragDrop.hxx"
#include "platform/x11/desktop/X11Theme.hxx"
#include "platform/x11/input/X11Joystick.hxx"
#include "platform/x11/internal/X11Atoms.hxx"

bool X11Backend::initialize(const VeraAppInfo& info) {
    XInitThreads();

    std::setlocale(LC_ALL, "");
    XSetLocaleModifiers("");

    m_ctx.display = XOpenDisplay(nullptr);
    if (!m_ctx.display) return false;

    m_ctx.xim = XOpenIM(m_ctx.display, nullptr, nullptr, nullptr);
    if (!m_ctx.xim) {
        XSetLocaleModifiers("@im=none");
        m_ctx.xim = XOpenIM(m_ctx.display, nullptr, nullptr, nullptr);
    }

    m_ctx.screen = DefaultScreen(m_ctx.display);
    m_ctx.root = RootWindow(m_ctx.display, m_ctx.screen);

    internAtomsX11(m_ctx);
    initializeXKBX11(m_ctx);
    m_hasXInput2 = initializeXInputX11(m_ctx, m_xinput2Opcode);
    initializeMonitorX11(m_ctx);
    initializeClipboardX11(m_ctx);
    initializeThemeX11(m_ctx);

    initializeJoystickX11(m_ctx);

    if (info.enablePlatformDebugging) {
        XSynchronize(m_ctx.display, True);
    }

    return true;
}

X11Backend::~X11Backend() {
    if (m_ctx.display) {
        shutdownCursorX11(m_ctx);
        if (m_ctx.clipboardOwnerWindow) {
            XDestroyWindow(m_ctx.display, m_ctx.clipboardOwnerWindow);
        }

        if (m_ctx.xim) {
            XCloseIM(m_ctx.xim);
            m_ctx.xim = nullptr;
        }

        shutdownJoystickX11(m_ctx);
        XCloseDisplay(m_ctx.display);
    }
}

std::expected<std::unique_ptr<VeraWindow>, VeraError> X11Backend::createWindow(
    const VeraWindowInfo& info) {
    VeraMonitorInfo targetMonitor = getPrimaryMonitorX11(m_ctx);
    auto monitors = getMonitorsX11(m_ctx);
    if (info.monitorIndex >= 0 &&
        static_cast<size_t>(info.monitorIndex) < monitors.size()) {
        targetMonitor = monitors[static_cast<size_t>(info.monitorIndex)];
    }

    int32_t x, y;
    if (info.x && info.y) {
        x = static_cast<int32_t>(*info.x);
        y = static_cast<int32_t>(*info.y);
    } else if (info.centerOnMonitor) {
        x = targetMonitor.workAreaX +
            (static_cast<int32_t>(targetMonitor.workAreaWidth) -
             static_cast<int32_t>(info.width)) /
                2;
        y = targetMonitor.workAreaY +
            (static_cast<int32_t>(targetMonitor.workAreaHeight) -
             static_cast<int32_t>(info.height)) /
                2;
    } else {
        x = targetMonitor.workAreaX;
        y = targetMonitor.workAreaY;
    }

    XSetWindowAttributes attributes{};
    attributes.background_pixel = WhitePixel(m_ctx.display, m_ctx.screen);
    attributes.override_redirect = False;
    ulong valueMask = CWBackPixel;

    int depth = DefaultDepth(m_ctx.display, m_ctx.screen);
    Visual* visual = DefaultVisual(m_ctx.display, m_ctx.screen);

    if (info.transparentFramebuffer) {
        XVisualInfo visualInfoTemplate{};
        visualInfoTemplate.screen = m_ctx.screen;
        visualInfoTemplate.depth = 32;
        visualInfoTemplate.c_class = TrueColor;
        int matchCount = 0;
        XVisualInfo* matches = XGetVisualInfo(
            m_ctx.display, VisualScreenMask | VisualDepthMask | VisualClassMask,
            &visualInfoTemplate, &matchCount);
        if (matches && matchCount > 0) {
            depth = matches[0].depth;
            visual = matches[0].visual;
            attributes.colormap =
                XCreateColormap(m_ctx.display, m_ctx.root, visual, AllocNone);
            valueMask |= CWColormap | CWBorderPixel;
            attributes.border_pixel = 0;
            XFree(matches);
        }
    }

    Window xid =
        XCreateWindow(m_ctx.display, m_ctx.root, x, y, info.width, info.height,
                      0, depth, InputOutput, visual, valueMask, &attributes);
    if (!xid) {
        return std::unexpected(VeraError{VeraErrorType::WindowCreationFailed,
                                         "XCreateWindow failed"});
    }

    VeraWindowHandle handle = m_ctx.allocateHandle();
    return std::make_unique<X11Window>(m_ctx, xid, handle, info);
}

void X11Backend::pollEvents() {
    updateJoystickX11(m_ctx);
    pollEventsX11(m_ctx, m_quitRequestCallback, m_displayChangeCallback);

    auto it = m_ctx.windowsByXid.begin();
    while (it != m_ctx.windowsByXid.end()) {
        X11Window* window = it->second;

        if (window && window->isPendingDeletion()) {
            it = m_ctx.windowsByXid.erase(it);
            delete window;
        } else {
            ++it;
        }
    }
}

void X11Backend::waitEvents() {
    waitForEventsX11(m_ctx, m_quitRequestCallback, m_displayChangeCallback);
}

void X11Backend::waitEventsTimeout(double timeoutSeconds) {
    waitForEventsWithTimeoutX11(m_ctx, timeoutSeconds, m_quitRequestCallback,
                                m_displayChangeCallback);
}

void X11Backend::setQuitRequestCallback(std::function<bool()> callback) {
    m_quitRequestCallback = std::move(callback);
}
void X11Backend::setDisplayChangeCallback(std::function<void()> callback) {
    m_displayChangeCallback = std::move(callback);
}
void X11Backend::setSystemThemeChangeCallback(
    std::function<void(VeraSystemTheme)> callback) {
    setThemeChangeCallbackX11(std::move(callback));
}

std::vector<VeraMonitorInfo> X11Backend::getMonitors() const {
    return getMonitorsX11(m_ctx);
}
VeraMonitorInfo X11Backend::getPrimaryMonitor() const {
    return getPrimaryMonitorX11(m_ctx);
}
VeraMonitorInfo X11Backend::getMonitorAt(int32_t x, int32_t y) const {
    return getMonitorAtCoordinateXYX11(m_ctx, x, y);
}
std::vector<VeraDisplayModeInfo> X11Backend::getSupportedDisplayModes(
    const VeraMonitorInfo& monitor) const {
    return getSupportedDisplayModesX11(m_ctx, monitor);
}

bool X11Backend::supportsNativeDecorationHitTesting() const { return false; }

std::string X11Backend::getClipboardText() const {
    return getClipboardTextX11(m_ctx);
}
void X11Backend::setClipboardText(const std::string& text) {
    setClipboardTextX11(m_ctx, text);
}
bool X11Backend::hasClipboardText() const { return hasClipboardTextX11(m_ctx); }

void X11Backend::setDragCallback(VeraDragCallback callback) {
    setCallback(std::move(callback));
}

VeraSystemTheme X11Backend::getSystemTheme() const {
    return getCurrentThemeX11(m_ctx);
}

std::vector<VeraInputDeviceInfo> X11Backend::getInputDevices() const {
    if (!m_hasXInput2) return {};
    return enumerateInputDevicesX11(m_ctx);
}

VeraNativeHandle X11Backend::getNativeHandle() const {
    VeraNativeHandle handle;
    handle.display = m_ctx.display;
    return handle;
}

void X11Backend::applySettings(VeraSettings settings) {
    m_ctx.keyRepeatDelay = settings.keyRepeatSettings.delayMs;
    m_ctx.keyRepeatRate = settings.keyRepeatSettings.rate;
}
