#include "WaylandPointer.hxx"

#include <linux/input-event-codes.h>

#include "core/app/Types.h"
#include "platform/wayland/window/WaylandWindow.hxx"

static double sPointerX = 0.0;
static double sPointerY = 0.0;
static WaylandWindow* sPointerTargetWindow = nullptr;

static VeraMouseButton translateMouseButton(uint32_t button) {
    switch (button) {
        case BTN_LEFT:
            return VeraMouseButton::Left;
        case BTN_RIGHT:
            return VeraMouseButton::Right;
        case BTN_MIDDLE:
            return VeraMouseButton::Middle;
        case BTN_SIDE:
            return VeraMouseButton::VeraButton4;
        case BTN_EXTRA:
            return VeraMouseButton::VeraButton5;
        default:
            return VeraMouseButton::Count;
    }
}

static void pointerHandleEnter(void* data, wl_pointer* pointer, uint32_t serial,
                               wl_surface* surface, wl_fixed_t sx,
                               wl_fixed_t sy) {
    auto* ctx = static_cast<WaylandContext*>(data);
    (void)pointer;
    (void)serial;

    auto it = ctx->windowsBySurface.find(surface);
    if (it != ctx->windowsBySurface.end()) {
        sPointerTargetWindow = it->second;
        sPointerX = wl_fixed_to_double(sx);
        sPointerY = wl_fixed_to_double(sy);
    }
}

static void pointerHandleLeave(void* data, wl_pointer* pointer, uint32_t serial,
                               wl_surface* surface) {
    (void)data;
    (void)pointer;
    (void)serial;
    (void)surface;
    sPointerTargetWindow = nullptr;
}

static void pointerHandleMotion(void* data, wl_pointer* pointer, uint32_t time,
                                wl_fixed_t sx, wl_fixed_t sy) {
    (void)data;
    (void)pointer;
    (void)time;
    sPointerX = wl_fixed_to_double(sx);
    sPointerY = wl_fixed_to_double(sy);
}

static void pointerHandleButton(void* data, wl_pointer* pointer,
                                uint32_t serial, uint32_t time, uint32_t button,
                                uint32_t state) {
    auto* ctx = static_cast<WaylandContext*>(data);
    (void)pointer;
    (void)serial;
    (void)time;
    (void)ctx;

    if (!sPointerTargetWindow) return;

    VeraMouseButton veraButton = translateMouseButton(button);
    bool pressed = (state == WL_POINTER_BUTTON_STATE_PRESSED);

    if (sPointerTargetWindow->getMouseButtonCallback()) {
        sPointerTargetWindow->getMouseButtonCallback()(veraButton, pressed);
    }
}

static void pointerHandleAxis(void* data, wl_pointer* pointer, uint32_t time,
                              uint32_t axis, wl_fixed_t value) {
    (void)data;
    (void)pointer;
    (void)time;

    if (!sPointerTargetWindow) return;

    double scrollVal = wl_fixed_to_double(value);
    double normScroll = -scrollVal / 10.0;

    if (sPointerTargetWindow->getScrollCallback()) {
        if (axis == WL_POINTER_AXIS_VERTICAL_SCROLL) {
            sPointerTargetWindow->getScrollCallback()(0.0, normScroll);
        } else if (axis == WL_POINTER_AXIS_HORIZONTAL_SCROLL) {
            sPointerTargetWindow->getScrollCallback()(normScroll, 0.0);
        }
    }
}

static void pointerHandleFrame(void* data, wl_pointer* pointer) {
    (void)data;
    (void)pointer;

    if (sPointerTargetWindow && sPointerTargetWindow->getMouseMoveCallback()) {
        sPointerTargetWindow->getMouseMoveCallback()(sPointerX, sPointerY);
    }
}

static void pointerHandleAxisSource(void* data, wl_pointer* pointer,
                                    uint32_t axis_source) {
    (void)data;
    (void)pointer;
    (void)axis_source;
}

static void pointerHandleAxisStop(void* data, wl_pointer* pointer,
                                  uint32_t time, uint32_t axis) {
    (void)data;
    (void)pointer;
    (void)time;
    (void)axis;
}

static void pointerHandleAxisDiscrete(void* data, wl_pointer* pointer,
                                      uint32_t axis, int32_t discrete) {
    (void)data;
    (void)pointer;
    (void)axis;
    (void)discrete;
}

static const wl_pointer_listener KPOINTER_LISTENER = {
    .enter = pointerHandleEnter,
    .leave = pointerHandleLeave,
    .motion = pointerHandleMotion,
    .button = pointerHandleButton,
    .axis = pointerHandleAxis,
    .frame = pointerHandleFrame,
    .axis_source = pointerHandleAxisSource,
    .axis_stop = pointerHandleAxisStop,
    .axis_discrete = pointerHandleAxisDiscrete,
    .axis_value120 = nullptr,
    .axis_relative_direction = nullptr,
    .warp = nullptr};

void addListenerToPointer(WaylandContext& ctx, wl_pointer* pointer) {
    if (pointer) {
        wl_pointer_add_listener(pointer, &KPOINTER_LISTENER, &ctx);
    }
}
