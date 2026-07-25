#include "WaylandCursor.hxx"

#include <wayland-cursor.h>
#include <wayland-server.h>

#include "platform/wayland/input/constraints/WaylandInputConstraint.hxx"

static VeraCursorMode sCurrentMode = VeraCursorMode::Normal;

// void advanceCursorFrame(WaylandContext& ctx) {
//     if (!ctx.currentCursor || ctx.currentCursor->image_count <= 1 ||
//     !ctx.pointer) {
//         return;
//     }
//
//     // Move to the next frame, looping back to 0 at the end
//     ctx.currentFrameIndex = (ctx.currentFrameIndex + 1) %
//     ctx.currentCursor->image_count;
//
//     wl_cursor_image* image =
//     ctx.currentCursor->images[ctx.currentFrameIndex]; wl_buffer* buffer =
//     wl_cursor_image_get_buffer(image);
//
//     // Attach the new frame's buffer
//     wl_surface_attach(ctx.cursorSurface, buffer, 0, 0);
//     wl_surface_damage(ctx.cursorSurface, 0, 0, image->width, image->height);
//     wl_surface_commit(ctx.cursorSurface);
//
//     // Note: The serial is only strictly required on .enter or .button.
//     // Frame updates during an active session can use the last cached
//     pointerSerial. wl_pointer_set_cursor(ctx.pointer, ctx.pointerSerial,
//     ctx.cursorSurface,
//                           image->hotspot_x, image->hotspot_y);
//
//     // Schedule the next frame using the cursor's native millisecond delay
//     uint32_t delayMs = image->delay;
//
//     // TODO: Trigger your engine's timer callback here to run
//     // advanceCursorFrame(ctx) again after 'delayMs' milliseconds.
// }

// 2. The structural logic that commits directly to the compositor
void applyCursorToWayland(WaylandContext& ctx, uint32_t serial) {
    if (!ctx.pointer) return;

    const char* cursorName = "left_ptr";
    switch (ctx.pendingShape) {
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

    if (!ctx.cursorTheme) {
        ctx.cursorTheme = wl_cursor_theme_load(nullptr, 24, ctx.shm);
    }
    if (!ctx.cursorTheme) return;

    wl_cursor* cursor = wl_cursor_theme_get_cursor(ctx.cursorTheme, cursorName);
    if (cursor && cursor->image_count > 0) {
        wl_cursor_image* image = cursor->images[0];
        wl_buffer* buffer = wl_cursor_image_get_buffer(image);

        if (!ctx.cursorSurface) {
            ctx.cursorSurface = wl_compositor_create_surface(ctx.compositor);
        }

        // Essential Wayland Order: Attach -> Damage -> Commit -> Set
        wl_surface_attach(ctx.cursorSurface, buffer, 0, 0);
        wl_surface_damage(ctx.cursorSurface, 0, 0, image->width, image->height);
        wl_surface_commit(ctx.cursorSurface);

        // Crucial: Pass the exact live serial received from the callback
        wl_pointer_set_cursor(ctx.pointer, serial, ctx.cursorSurface,
                              image->hotspot_x, image->hotspot_y);
    }
}

// 1. Your app calls this to change state
void setCursorShapeWayland(WaylandContext& ctx, VeraCursorShape shape) {
    ctx.pendingShape = shape;

    // If the pointer is already active inside our surface, update it
    // immediately
    if (ctx.pointer && ctx.pointerSerial != 0) {
        applyCursorToWayland(ctx, ctx.pointerSerial);
    }
}

void setCursorModeWayland(WaylandContext& ctx, wl_surface* surface,
                          VeraCursorMode mode) {
    sCurrentMode = mode;

    if (mode == VeraCursorMode::Disabled) {
        lockPointerWayland(ctx, surface);

        if (ctx.pointer) {
            wl_pointer_set_cursor(ctx.pointer, ctx.pointerSerial, nullptr, 0,
                                  0);
        }
    } else {
        unlockPointerWayland(ctx);

        if (mode == VeraCursorMode::Hidden) {
            if (ctx.pointer) {
                wl_pointer_set_cursor(ctx.pointer, ctx.pointerSerial, nullptr,
                                      0, 0);
            }
        } else {
            setCursorShapeWayland(ctx, VeraCursorShape::Arrow);
        }
    }
}
