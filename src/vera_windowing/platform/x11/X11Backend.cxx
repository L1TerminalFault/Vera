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

// // Internal conversion mapping platform-specific enum entries to linux joydev
// // button numbers
// static constexpr uint8_t mapToLinuxJoydevButton(VeraJoystickButton button) {
//     switch (button) {
//         // Face Buttons
//         case VeraJoystickButton::Cross:
//         case VeraJoystickButton::XboxA:
//             return 0;
//         case VeraJoystickButton::Circle:
//         case VeraJoystickButton::XboxB:
//             return 1;
//         case VeraJoystickButton::Square:
//         case VeraJoystickButton::XboxX:
//             return 2;
//         case VeraJoystickButton::Triangle:
//         case VeraJoystickButton::XboxY:
//             return 3;
//
//         // Bumpers
//         case VeraJoystickButton::L1:
//         case VeraJoystickButton::XboxLB:
//             return 4;
//         case VeraJoystickButton::R1:
//         case VeraJoystickButton::XboxRB:
//             return 5;
//
//         // Triggers (Digital/Click Emulation)
//         case VeraJoystickButton::L2:
//         case VeraJoystickButton::XboxLT:
//             return 6;
//         case VeraJoystickButton::R2:
//         case VeraJoystickButton::XboxRT:
//             return 7;
//
//         // Menu / System Options
//         case VeraJoystickButton::Share:
//         case VeraJoystickButton::XboxBack:
//             return 8;
//         case VeraJoystickButton::Options:
//         case VeraJoystickButton::XboxStart:
//             return 9;
//         case VeraJoystickButton::PS:
//         case VeraJoystickButton::XboxGuide:
//             return 10;
//
//         // Stick Clicks
//         case VeraJoystickButton::L3:
//         case VeraJoystickButton::XboxLS:
//             return 11;
//         case VeraJoystickButton::R3:
//         case VeraJoystickButton::XboxRS:
//             return 12;
//
//         // Directional D-Pad Fallbacks
//         case VeraJoystickButton::DpadUp:
//             return 13;
//         case VeraJoystickButton::DpadDown:
//             return 14;
//         case VeraJoystickButton::DpadLeft:
//             return 15;
//         case VeraJoystickButton::DpadRight:
//             return 16;
//
//         // Platform Exclusives
//         case VeraJoystickButton::Touchpad:
//             return 17;
//         case VeraJoystickButton::XboxShare:
//             return 18;
//
//         default:
//             return 255;
//     }
// }
//
// bool X11Backend::isPressed(VeraPressable input) const {
//     return std::visit(
//         [this](auto&& arg) -> bool {
//             using T = std::decay_t<decltype(arg)>;
//
//             // 1. Keyboard Input Evaluation
//             if constexpr (std::is_same_v<T, VeraKey>) {
//                 if (arg == VeraKey::Unknown || arg == VeraKey::Count) {
//                     return false;
//                 }
//                 size_t keyIndex = static_cast<size_t>(arg);
//                 if (keyIndex < std::size(m_ctx.keyStates)) {
//                     return m_ctx.keyStates[keyIndex];
//                 }
//                 return false;
//             }
//
//             // 2. Mouse Input Evaluation
//             else if constexpr (std::is_same_v<T, VeraMouseButton>) {
//                 if (arg == VeraMouseButton::Count) {
//                     return false;
//                 }
//                 size_t mouseIndex = static_cast<size_t>(arg);
//                 if (mouseIndex < std::size(m_ctx.mouseButtonStates)) {
//                     return m_ctx.mouseButtonStates[mouseIndex];
//                 }
//                 return false;
//             }
//
//             // 3. Gamepad Input Evaluation via Member Context
//             else if constexpr (std::is_same_v<T, VeraJoystickButton>) {
//                 if (arg == VeraJoystickButton::Count) {
//                     return false;
//                 }
//
//                 uint8_t rawButtonId = mapToLinuxJoydevButton(arg);
//                 if (rawButtonId == 255) {
//                     return false;
//                 }
//
//                 // Directly query Slot 0 from the m_ctx encapsulation array
//                 if (!m_ctx.joysticks.empty()) {
//                     const auto& primaryJoy = m_ctx.joysticks[0];
//                     if (primaryJoy.fd >= 0 &&
//                         rawButtonId < primaryJoy.state.buttons.size()) {
//                         return primaryJoy.state.buttons[rawButtonId];
//                     }
//                 }
//                 return false;
//             }
//
//             return false;
//         },
//         input);
// }
