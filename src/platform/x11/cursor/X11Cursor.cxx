#include "X11Cursor.hxx"

namespace cursor {

static std::unordered_map<VeraCursorShape, Cursor> gShapeCache;
static Cursor gBlankCursor = 0;

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
    if (gBlankCursor) return gBlankCursor;

    char data[1] = {0};
    Pixmap blankPixmap = XCreateBitmapFromData(ctx.display, window, data, 1, 1);
    XColor black{};
    gBlankCursor = XCreatePixmapCursor(ctx.display, blankPixmap, blankPixmap,
                                       &black, &black, 0, 0);
    XFreePixmap(ctx.display, blankPixmap);
    return gBlankCursor;
}

void applyShape(X11Context& ctx, Window window, VeraCursorShape shape) {
    auto it = gShapeCache.find(shape);
    Cursor cursor;
    if (it != gShapeCache.end()) {
        cursor = it->second;
    } else {
        cursor = XCreateFontCursor(ctx.display, shapeToFontGlyph(shape));
        gShapeCache[shape] = cursor;
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
            break;
    }
}

void shutdown(X11Context& ctx) {
    for (auto& [shape, cursor] : gShapeCache) {
        XFreeCursor(ctx.display, cursor);
    }
    gShapeCache.clear();
    if (gBlankCursor) {
        XFreeCursor(ctx.display, gBlankCursor);
        gBlankCursor = 0;
    }
}

}  // namespace cursor
