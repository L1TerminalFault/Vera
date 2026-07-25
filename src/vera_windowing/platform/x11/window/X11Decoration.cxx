#include "X11Decoration.hxx"

#include <X11/Xlib.h>

#include <cstdint>

#include "core/app/Types.h"
#include "platform/x11/internal/X11Internal.hxx"

struct MotifWmHints {
    ulong flags;
    ulong functions;
    ulong decorations;
    int64_t inputMode;
    ulong status;
};

static constexpr ulong MWM_HINTS_DECORATIONS = 1L << 1;

void setDecoratedX11(X11Context& ctx, Window window, bool decorated) {
    MotifWmHints hints{};
    hints.flags = MWM_HINTS_DECORATIONS;
    hints.decorations = decorated ? 1 : 0;

    XChangeProperty(ctx.display, window, ctx.atoms.motifWmHints,
                    ctx.atoms.motifWmHints, 32, PropModeReplace,
                    reinterpret_cast<unsigned char*>(&hints), 5);
}

static bool pointInRect(int32_t x, int32_t y, const VeraRect& r) {
    return x >= static_cast<int32_t>(r.x) &&
           x < static_cast<int32_t>(r.x + r.width) &&
           y >= static_cast<int32_t>(r.y) &&
           y < static_cast<int32_t>(r.y + r.height);
}

void handleTitlebarButtonPressX11(X11Context& ctx, Window window,
                                  const VeraHitTestRegions& regions,
                                  int32_t clickX, int32_t clickY) {
    if (!regions.dragRegion ||
        !pointInRect(clickX, clickY, *regions.dragRegion)) {
        return;
    }

    if (regions.minimizeButton &&
        pointInRect(clickX, clickY, *regions.minimizeButton)) {
        return;
    }
    if (regions.maximizeButton &&
        pointInRect(clickX, clickY, *regions.maximizeButton)) {
        return;
    }
    if (regions.closeButton &&
        pointInRect(clickX, clickY, *regions.closeButton)) {
        return;
    }

    constexpr int64_t moveResizeMove = 8;

    Window child;
    int rootX = 0, rootY = 0, winX = 0, winY = 0;
    unsigned int mask;
    ::Window rootReturn;
    XQueryPointer(ctx.display, window, &rootReturn, &child, &rootX, &rootY,
                  &winX, &winY, &mask);

    XEvent event{};
    event.type = ClientMessage;
    event.xclient.window = window;
    event.xclient.message_type = ctx.atoms.netWmMoveresize;
    event.xclient.format = 32;
    event.xclient.data.l[0] = rootX;
    event.xclient.data.l[1] = rootY;
    event.xclient.data.l[2] = moveResizeMove;
    event.xclient.data.l[3] = Button1;
    event.xclient.data.l[4] = 1;

    XUngrabPointer(ctx.display, CurrentTime);
    XSendEvent(ctx.display, ctx.root, False,
               SubstructureRedirectMask | SubstructureNotifyMask, &event);
}
