#include "platform/wayland/WaylandBackend.hxx"

#include <algorithm>
#include <cstring>
#include <iostream>

#include "core/app/Types.h"
#include "platform/wayland/desktop/WaylandClipboard.hxx"
#include "platform/wayland/desktop/WaylandDragDrop.hxx"
#include "platform/wayland/desktop/WaylandTheme.hxx"
#include "platform/wayland/events/WaylandEvents.hxx"
#include "platform/wayland/input/WaylandJoystick.hxx"
#include "platform/wayland/input/WaylandKeyboard.hxx"
#include "platform/wayland/input/WaylandPointer.hxx"
#include "platform/wayland/input/constraints/WaylandInputConstraint.hxx"
#include "platform/wayland/monitor/WaylandMonitor.hxx"
#include "platform/wayland/window/WaylandWindow.hxx"

static void seatHandleCapabilities(void* data, wl_seat* seat,
                                   uint32_t capabilities) {
    auto* ctx = static_cast<WaylandContext*>(data);

    if ((capabilities & WL_SEAT_CAPABILITY_POINTER) && !ctx->pointer) {
        ctx->pointer = wl_seat_get_pointer(seat);
        addListenerToPointer(*ctx, ctx->pointer);
    } else if (!(capabilities & WL_SEAT_CAPABILITY_POINTER) && ctx->pointer) {
        wl_pointer_release(ctx->pointer);
        ctx->pointer = nullptr;
    }

    if ((capabilities & WL_SEAT_CAPABILITY_KEYBOARD) && !ctx->keyboard) {
        ctx->keyboard = wl_seat_get_keyboard(seat);
        addListenerToKeyboard(*ctx, ctx->keyboard);
    } else if (!(capabilities & WL_SEAT_CAPABILITY_KEYBOARD) && ctx->keyboard) {
        wl_keyboard_release(ctx->keyboard);
        ctx->keyboard = nullptr;
    }
}

static void seatHandleName(void* data, wl_seat* seat, const char* name) {
    (void)data;
    (void)seat;
    (void)name;
#ifdef DEBUG
    std::cout << "[Wayland] Active seat group identified: " << name << "\n";
#endif
}

static void wmBaseHandlePing(void* data, xdg_wm_base* wmBase, uint32_t serial) {
    (void)data;
    xdg_wm_base_pong(wmBase, serial);
}

static const wl_seat_listener KSEAT_LISTENER = {
    .capabilities = seatHandleCapabilities, .name = seatHandleName};

static const xdg_wm_base_listener KWM_BASE_LISTENER = {.ping =
                                                           wmBaseHandlePing};

static void registryHandleGlobal(void* data, wl_registry* registry,
                                 uint32_t name, const char* interface,
                                 uint32_t version) {
    auto* ctx = static_cast<WaylandContext*>(data);

    if (std::strcmp(interface, "wl_compositor") == 0) {
        ctx->compositor = static_cast<wl_compositor*>(wl_registry_bind(
            registry, name, &wl_compositor_interface, std::min(version, 4u)));
    } else if (std::strcmp(interface, "wl_shm") == 0) {
        ctx->shm = static_cast<wl_shm*>(
            wl_registry_bind(registry, name, &wl_shm_interface, 1));
    } else if (std::strcmp(interface, "wl_seat") == 0) {
        ctx->seat = static_cast<wl_seat*>(wl_registry_bind(
            registry, name, &wl_seat_interface, std::min(version, 7u)));
        wl_seat_add_listener(ctx->seat, &KSEAT_LISTENER, ctx);
    } else if (std::strcmp(interface, "xdg_wm_base") == 0) {
        ctx->wmBase = static_cast<xdg_wm_base*>(
            wl_registry_bind(registry, name, &xdg_wm_base_interface, 1));

        xdg_wm_base_add_listener(ctx->wmBase, &KWM_BASE_LISTENER, nullptr);
    } else if (std::strcmp(interface, "wp_pointer_constraints_v1") == 0) {
        ctx->pointerConstraints =
            static_cast<zwp_pointer_constraints_v1*>(wl_registry_bind(
                registry, name, &zwp_pointer_constraints_v1_interface, 1));
    } else if (std::strcmp(interface, "zwp_relative_pointer_manager_v1") == 0) {
        ctx->relativePointerManager =
            static_cast<zwp_relative_pointer_manager_v1*>(wl_registry_bind(
                registry, name, &zwp_relative_pointer_manager_v1_interface, 1));
    }
}

static void registryHandleGlobalRemove(void* data, wl_registry* registry,
                                       uint32_t name) {
    (void)data;
    (void)registry;
    (void)name;
}

static const wl_registry_listener KREGISTRY_LISTENER = {
    .global = registryHandleGlobal,
    .global_remove = registryHandleGlobalRemove};

WaylandBackend::~WaylandBackend() { shutdown(); }

bool WaylandBackend::initialize(const VeraAppInfo& info) {
    (void)info;

    m_ctx.display = wl_display_connect(nullptr);
    if (!m_ctx.display) {
        return false;
    }

    m_ctx.registry = wl_display_get_registry(m_ctx.display);
    wl_registry_add_listener(m_ctx.registry, &KREGISTRY_LISTENER, &m_ctx);

    wl_display_roundtrip(m_ctx.display);

    if (!m_ctx.compositor || !m_ctx.wmBase) {
        std::cerr << "[Wayland] Essential shell interfaces missing.\n";
        shutdown();
        return false;
    }

    initializeThemeWayland(m_ctx);
    initializeJoystickWayland(m_ctx);

    return true;
}

void WaylandBackend::shutdown() {
    shutdownJoystickWayland(m_ctx);
    shutdownThemeWayland();
    unlockPointerWayland(m_ctx);

    if (m_ctx.wmBase) {
        xdg_wm_base_destroy(m_ctx.wmBase);
        m_ctx.wmBase = nullptr;
    }
    if (m_ctx.compositor) {
        wl_compositor_destroy(m_ctx.compositor);
        m_ctx.compositor = nullptr;
    }
    if (m_ctx.shm) {
        wl_shm_destroy(m_ctx.shm);
        m_ctx.shm = nullptr;
    }
    if (m_ctx.seat) {
        wl_seat_destroy(m_ctx.seat);
        m_ctx.seat = nullptr;
    }
    if (m_ctx.pointerConstraints) {
        zwp_pointer_constraints_v1_destroy(m_ctx.pointerConstraints);
        m_ctx.pointerConstraints = nullptr;
    }
    if (m_ctx.relativePointerManager) {
        zwp_relative_pointer_manager_v1_destroy(m_ctx.relativePointerManager);
        m_ctx.relativePointerManager = nullptr;
    }
    if (m_ctx.registry) {
        wl_registry_destroy(m_ctx.registry);
        m_ctx.registry = nullptr;
    }
    if (m_ctx.display) {
        wl_display_disconnect(m_ctx.display);
        m_ctx.display = nullptr;
    }
}

std::expected<std::unique_ptr<VeraWindow>, VeraError>
WaylandBackend::createWindow(const VeraWindowInfo& info) {
    VeraWindowHandle handle =
        static_cast<VeraWindowHandle>(m_ctx.allocateHandle());

    try {
        auto window = std::make_unique<WaylandWindow>(m_ctx, handle, info);
        return window;
    } catch (const std::exception& e) {
        return std::unexpected(VeraError{
            VeraErrorType::WindowCreationFailed,
            std::string("Failed to create Wayland window: ") + e.what()});
    }
}

void WaylandBackend::pollEvents() {
    updateJoystickWayland(m_ctx);
    updateThemeWayland();

    pollEventsWayland(m_ctx, m_quitRequestCallback, m_displayChangeCallback);

    auto it = m_ctx.windowsBySurface.begin();
    while (it != m_ctx.windowsBySurface.end()) {
        WaylandWindow* window = it->second;

        if (window && window->isPendingDeletion()) {
            it = m_ctx.windowsBySurface.erase(it);

            delete window;
        } else {
            ++it;
        }
    }

    if (m_ctx.display) {
        wl_display_flush(m_ctx.display);
    }
}

void WaylandBackend::waitEvents() {
    updateJoystickWayland(m_ctx);
    updateThemeWayland();
    waitForEventsWayland(m_ctx, m_quitRequestCallback, m_displayChangeCallback);
}

void WaylandBackend::waitEventsTimeout(double timeoutSeconds) {
    updateJoystickWayland(m_ctx);
    updateThemeWayland();
    waitForEventsWithTimeoutWayland(
        m_ctx, timeoutSeconds, m_quitRequestCallback, m_displayChangeCallback);
}

void WaylandBackend::setQuitRequestCallback(std::function<bool()> callback) {
    m_quitRequestCallback = callback;
}

void WaylandBackend::setDisplayChangeCallback(std::function<void()> callback) {
    m_displayChangeCallback = callback;
}

void WaylandBackend::setSystemThemeChangeCallback(
    std::function<void(VeraSystemTheme)> callback) {
    m_systemThemeChangeCallback = callback;
}

std::vector<VeraMonitorInfo> WaylandBackend::getMonitors() const {
    return getMonitorsWayland(m_ctx);
}

VeraMonitorInfo WaylandBackend::getPrimaryMonitor() const {
    return getPrimaryMonitorWayland(m_ctx);
}

VeraMonitorInfo WaylandBackend::getMonitorAt(int32_t x, int32_t y) const {
    return getMonitorAtCoordinateXYWayland(m_ctx, x, y);
}

std::vector<VeraDisplayModeInfo> WaylandBackend::getSupportedDisplayModes(
    const VeraMonitorInfo& monitor) const {
    return getSupportedDisplayModesWayland(m_ctx, monitor);
}

bool WaylandBackend::supportsNativeDecorationHitTesting() const { return true; }

std::string WaylandBackend::getClipboardText() const {
    return getClipboardTextWayland(m_ctx);
}

void WaylandBackend::setClipboardText(const std::string& text) {
    setClipboardTextWayland(m_ctx, text);
}

bool WaylandBackend::hasClipboardText() const {
    return hasClipboardTextWayland(m_ctx);
}

void WaylandBackend::setDragCallback(VeraDragCallback callback) {
    setDragCallbackWayland(m_ctx, callback);
}

VeraSystemTheme WaylandBackend::getSystemTheme() const {
    return (getActiveThemeModeWayland() == VeraThemeMode::Dark)
               ? VeraSystemTheme::Dark
               : VeraSystemTheme::Light;
}

std::vector<VeraInputDeviceInfo> WaylandBackend::getInputDevices() const {
    return {};
}

VeraNativeHandle WaylandBackend::getNativeHandle() const {
    VeraNativeHandle handle{};
    handle.display = m_ctx.display;
    return handle;
}

void WaylandBackend::applySettings(VeraSettings settings) {
    m_ctx.keyRepeatDelay = settings.keyRepeatSettings.delayMs;
    m_ctx.keyRepeatRate = settings.keyRepeatSettings.rate;
}

// Internal conversion mapping platform-specific enum entries to linux joydev
// button numbers
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

// bool WaylandBackend::isPressed(VeraPressable input) const {
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
