#include "X11Cursor.hxx"

namespace vera::x11::cursor {

using namespace core::input;
using namespace internal;

static std::unordered_map<VeraCursorShape, Cursor> g_shapeCache;
static Cursor g_blankCursor = 0;

static unsigned int shapeToFontGlyph(VeraCursorShape shape) {
    switch (shape) {
        case VeraCursorShape::Arrow:
            return XC_left_ptr;
        case VeraCursorShape::IBeam:
            return XC_xterm;
        case VeraCursorShape::Crosshair:
            return XC_crosshair;
        case VeraCursorShape::Hand:
            return XC_hand2;
        case VeraCursorShape::HResize:
            return XC_sb_h_double_arrow;
        case VeraCursorShape::VResize:
            return XC_sb_v_double_arrow;
        case VeraCursorShape::CornerResizeNWSE:
            return XC_bottom_right_corner;
        case VeraCursorShape::CornerResizeNESW:
            return XC_bottom_left_corner;
        case VeraCursorShape::NotAllowed:
            return XC_X_cursor;
        default:
            return XC_left_ptr;
    }
}

static Cursor getOrCreateBlankCursor(X11Context& ctx, Window window) {
    if (g_blankCursor) return g_blankCursor;

    char data[1] = {0};
    Pixmap blankPixmap = XCreateBitmapFromData(ctx.display, window, data, 1, 1);
    XColor black{};
    g_blankCursor = XCreatePixmapCursor(ctx.display, blankPixmap, blankPixmap,
                                        &black, &black, 0, 0);
    XFreePixmap(ctx.display, blankPixmap);
    return g_blankCursor;
}

void applyShape(X11Context& ctx, Window window, VeraCursorShape shape) {
    auto it = g_shapeCache.find(shape);
    Cursor cursor;
    if (it != g_shapeCache.end()) {
        cursor = it->second;
    } else {
        cursor = XCreateFontCursor(ctx.display, shapeToFontGlyph(shape));
        g_shapeCache[shape] = cursor;
    }
    XDefineCursor(ctx.display, window, cursor);
}

void applyMode(X11Context& ctx, Window window, VeraCursorMode mode) {
    switch (mode) {
        case VeraCursorMode::Normal:
            XUngrabPointer(ctx.display, CurrentTime);
            XUndefineCursor(ctx.display, window);
            break;
        case VeraCursorMode::Hidden:
            XUngrabPointer(ctx.display, CurrentTime);
            XDefineCursor(ctx.display, window,
                          getOrCreateBlankCursor(ctx, window));
            break;
        case VeraCursorMode::Disabled:
            XDefineCursor(ctx.display, window,
                          getOrCreateBlankCursor(ctx, window));
            XGrabPointer(
                ctx.display, window, True,
                ButtonPressMask | ButtonReleaseMask | PointerMotionMask,
                GrabModeAsync, GrabModeAsync, window, None, CurrentTime);
            // The event loop is responsible for re-centering the pointer each
            // motion event and converting absolute deltas to relative ones
            // while this mode is active (see events/X11Events.cxx).
            break;
    }
}

void shutdown(X11Context& ctx) {
    for (auto& [shape, cursor] : g_shapeCache) {
        XFreeCursor(ctx.display, cursor);
    }
    g_shapeCache.clear();
    if (g_blankCursor) {
        XFreeCursor(ctx.display, g_blankCursor);
        g_blankCursor = 0;
    }
}

}  // namespace vera::x11::cursor
