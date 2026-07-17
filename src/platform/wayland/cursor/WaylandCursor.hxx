#pragma once

#include <wayland-cursor.h>

#include "core/window/Window.h"
#include "platform/wayland/input/constraints/WaylandInputConstraint.hxx"
#include "platform/wayland/internal/WaylandInternal.hxx"

namespace vera::wayland::cursor {

using namespace vera::core::input;
using namespace internal;
using namespace vera::wayland::inputconstraints;

static VeraCursorMode s_currentMode = VeraCursorMode::Normal;

void setShape(WaylandContext& ctx, VeraCursorShape shape) {
    if (s_currentMode != VeraCursorMode::Normal || !ctx.pointer) {
        return;
    }

    const char* cursorName = "left_ptr";
    switch (shape) {
        case VeraCursorShape::Arrow:
            cursorName = "left_ptr";
            break;
        case VeraCursorShape::IBeam:
            cursorName = "xterm";
            break;
        case VeraCursorShape::Crosshair:
            cursorName = "crosshair";
            break;
        case VeraCursorShape::Hand:
            cursorName = "pointer";
            break;
        case VeraCursorShape::HResize:
            cursorName = "sb_h_double_arrow";
            break;
        case VeraCursorShape::VResize:
            cursorName = "sb_v_double_arrow";
            break;
        case VeraCursorShape::CornerResizeNWSE:
            cursorName = "top_left_corner";
            break;
        case VeraCursorShape::CornerResizeNESW:
            cursorName = "top_right_corner";
            break;
        case VeraCursorShape::NotAllowed:
            cursorName = "not-allowed";
            break;
        default:
            break;
    }

    wl_cursor_theme* theme = wl_cursor_theme_load(nullptr, 24, ctx.shm);
    if (!theme) return;

    wl_cursor* cursor = wl_cursor_theme_get_cursor(theme, cursorName);
    if (cursor && cursor->image_count > 0) {
        wl_cursor_image* image = cursor->images[0];
        wl_buffer* buffer = wl_cursor_image_get_buffer(image);

        if (!ctx.cursorSurface) {
            ctx.cursorSurface = wl_compositor_create_surface(ctx.compositor);
        }

        wl_surface_attach(ctx.cursorSurface, buffer, 0, 0);
        wl_surface_damage(ctx.cursorSurface, 0, 0, image->width, image->height);
        wl_surface_commit(ctx.cursorSurface);

        wl_pointer_set_cursor(ctx.pointer, ctx.pointerSerial, ctx.cursorSurface,
                              image->hotspot_x, image->hotspot_y);
    }
    wl_cursor_theme_destroy(theme);
}

void setMode(WaylandContext& ctx, wl_surface* surface, VeraCursorMode mode) {
    s_currentMode = mode;

    if (mode == VeraCursorMode::Disabled) {
        lockPointer(ctx, surface);

        if (ctx.pointer) {
            wl_pointer_set_cursor(ctx.pointer, ctx.pointerSerial, nullptr, 0,
                                  0);
        }
    } else {
        unlockPointer(ctx);

        if (mode == VeraCursorMode::Hidden) {
            if (ctx.pointer) {
                wl_pointer_set_cursor(ctx.pointer, ctx.pointerSerial, nullptr,
                                      0, 0);
            }
        } else {
            setShape(ctx, VeraCursorShape::Arrow);
        }
    }
}

}  // namespace vera::wayland::cursor
