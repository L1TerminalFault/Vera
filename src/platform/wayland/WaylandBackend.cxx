#include "platform/wayland/WaylandBackend.hxx"

#include <algorithm>
#include <cstring>
#include <iostream>

#include "core/window/WindowTypes.h"
#include "platform/wayland/desktop/WaylandClipboard.hxx"
#include "platform/wayland/desktop/WaylandDragDrop.hxx"
#include "platform/wayland/desktop/WaylandTheme.hxx"
#include "platform/wayland/events/WaylandEvents.hxx"
#include "platform/wayland/input/WaylandJoystick.hxx"
#include "platform/wayland/input/WaylandKeyboard.hxx"
#include "platform/wayland/input/constraints/WaylandInputConstraint.hxx"
#include "platform/wayland/monitor/WaylandMonitor.hxx"
#include "platform/wayland/window/WaylandWindow.hxx"

namespace vera::wayland {

using namespace input;
using namespace desktop;
using namespace inputconstraints;
using namespace window;
using namespace monitor;

static void seatHandleCapabilities(void* data, wl_seat* seat,
                                   uint32_t capabilities) {
    auto* ctx = static_cast<WaylandContext*>(data);

    if ((capabilities & WL_SEAT_CAPABILITY_POINTER) && !ctx->pointer) {
        ctx->pointer = wl_seat_get_pointer(seat);
    } else if (!(capabilities & WL_SEAT_CAPABILITY_POINTER) && ctx->pointer) {
        wl_pointer_release(ctx->pointer);
        ctx->pointer = nullptr;
    }

    if ((capabilities & WL_SEAT_CAPABILITY_KEYBOARD) && !ctx->keyboard) {
        ctx->keyboard = wl_seat_get_keyboard(seat);
        wl_keyboard_add_listener(ctx->keyboard, &kKeyboardListener, ctx);
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

static const wl_seat_listener kSeatListener = {
    .capabilities = seatHandleCapabilities, .name = seatHandleName};

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
        wl_seat_add_listener(ctx->seat, &kSeatListener, ctx);
    } else if (std::strcmp(interface, "xdg_wm_base") == 0) {
        ctx->wmBase = static_cast<xdg_wm_base*>(
            wl_registry_bind(registry, name, &xdg_wm_base_interface, 1));
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

static const wl_registry_listener kRegistryListener = {
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
    wl_registry_add_listener(m_ctx.registry, &kRegistryListener, &m_ctx);

    wl_display_roundtrip(m_ctx.display);

    if (!m_ctx.compositor || !m_ctx.wmBase) {
        std::cerr << "[Wayland] Essential shell interfaces missing.\n";
        shutdown();
        return false;
    }

    theme::initialize(m_ctx);
    input::initialize(m_ctx);

    return true;
}

void WaylandBackend::shutdown() {
    input::shutdown(m_ctx);
    theme::shutdown();
    unlockPointer(m_ctx);

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

void WaylandBackend::setJoystickButtonCallback(
    VeraJoystickButtonCallback callback) {
    input::setButtonCallback(callback);
}

void WaylandBackend::setJoystickAxisCallback(
    VeraJoystickAxisCallback callback) {
    input::setAxisCallback(callback);
}

void WaylandBackend::pollEvents() {
    input::update(m_ctx);
    theme::update();

    events::poll(m_ctx, m_quitRequestCallback, m_displayChangeCallback);

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
}

void WaylandBackend::waitEvents() {
    input::update(m_ctx);
    theme::update();
    events::wait(m_ctx, m_quitRequestCallback, m_displayChangeCallback);
}

void WaylandBackend::waitEventsTimeout(double timeoutSeconds) {
    input::update(m_ctx);
    theme::update();
    events::waitTimeout(m_ctx, timeoutSeconds, m_quitRequestCallback,
                        m_displayChangeCallback);
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
    return monitor::getMonitors(m_ctx);
}

VeraMonitorInfo WaylandBackend::getPrimaryMonitor() const {
    return monitor::getPrimaryMonitor(m_ctx);
}

VeraMonitorInfo WaylandBackend::getMonitorAt(int32_t x, int32_t y) const {
    return monitor::getMonitorAt(m_ctx, x, y);
}

std::vector<VeraDisplayModeInfo> WaylandBackend::getSupportedDisplayModes(
    const VeraMonitorInfo& monitor) const {
    return monitor::getSupportedDisplayModes(m_ctx, monitor);
}

bool WaylandBackend::supportsNativeDecorationHitTesting() const { return true; }

std::string WaylandBackend::getClipboardText() const {
    return clipboard::getClipboardText(m_ctx);
}

void WaylandBackend::setClipboardText(const std::string& text) {
    clipboard::setClipboardText(m_ctx, text);
}

bool WaylandBackend::hasClipboardText() const {
    return clipboard::hasClipboardText(m_ctx);
}

void WaylandBackend::setDragCallback(VeraDragCallback callback) {
    dnd::setDragCallback(m_ctx, callback);
}

VeraSystemTheme WaylandBackend::getSystemTheme() const {
    return (theme::getActiveMode() == theme::VeraThemeMode::Dark)
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

}  // namespace vera::wayland
