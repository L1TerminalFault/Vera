#pragma once

#include <linux/input-event-codes.h>

#include "platform/wayland/window/WaylandWindow.hxx"

namespace vera::wayland::input {

using namespace core::input;
using namespace window;

static double s_pointerX = 0.0;
static double s_pointerY = 0.0;
static WaylandWindow* s_pointerTargetWindow = nullptr;

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
        s_pointerTargetWindow = it->second;
        s_pointerX = wl_fixed_to_double(sx);
        s_pointerY = wl_fixed_to_double(sy);
    }
}

static void pointerHandleLeave(void* data, wl_pointer* pointer, uint32_t serial,
                               wl_surface* surface) {
    (void)data;
    (void)pointer;
    (void)serial;
    (void)surface;
    s_pointerTargetWindow = nullptr;
}

static void pointerHandleMotion(void* data, wl_pointer* pointer, uint32_t time,
                                wl_fixed_t sx, wl_fixed_t sy) {
    (void)data;
    (void)pointer;
    (void)time;
    s_pointerX = wl_fixed_to_double(sx);
    s_pointerY = wl_fixed_to_double(sy);
}

static void pointerHandleButton(void* data, wl_pointer* pointer,
                                uint32_t serial, uint32_t time, uint32_t button,
                                uint32_t state) {
    auto* ctx = static_cast<WaylandContext*>(data);
    (void)pointer;
    (void)serial;
    (void)time;
    (void)ctx;

    if (!s_pointerTargetWindow) return;

    VeraMouseButton veraButton = translateMouseButton(button);
    bool pressed = (state == WL_POINTER_BUTTON_STATE_PRESSED);

    if (s_pointerTargetWindow->getMouseButtonCallback()) {
        s_pointerTargetWindow->getMouseButtonCallback()(veraButton, pressed);
    }
}

static void pointerHandleAxis(void* data, wl_pointer* pointer, uint32_t time,
                              uint32_t axis, wl_fixed_t value) {
    (void)data;
    (void)pointer;
    (void)time;

    if (!s_pointerTargetWindow) return;

    double scrollVal = wl_fixed_to_double(value);
    double normScroll = -scrollVal / 10.0;

    if (s_pointerTargetWindow->getScrollCallback()) {
        if (axis == WL_POINTER_AXIS_VERTICAL_SCROLL) {
            s_pointerTargetWindow->getScrollCallback()(0.0, normScroll);
        } else if (axis == WL_POINTER_AXIS_HORIZONTAL_SCROLL) {
            s_pointerTargetWindow->getScrollCallback()(normScroll, 0.0);
        }
    }
}

static void pointerHandleFrame(void* data, wl_pointer* pointer) {
    (void)data;
    (void)pointer;

    if (s_pointerTargetWindow &&
        s_pointerTargetWindow->getMouseMoveCallback()) {
        s_pointerTargetWindow->getMouseMoveCallback()(s_pointerX, s_pointerY);
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

static const wl_pointer_listener kPointerListener = {
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
};

void addListener(WaylandContext& ctx, wl_pointer* pointer) {
    if (pointer) {
        wl_pointer_add_listener(pointer, &kPointerListener, &ctx);
    }
}

}  // namespace vera::wayland::input
