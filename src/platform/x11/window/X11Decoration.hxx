#pragma once
#include <X11/Xlib.h>

#include "core/window/WindowTypes.h"
#include "platform/x11/internal/X11Internal.hxx"

namespace vera::x11::window {

using namespace internal;

struct MotifWmHints {
    ulong flags;
    ulong functions;
    ulong decorations;
    long inputMode;
    ulong status;
};

static constexpr ulong MWM_HINTS_DECORATIONS = 1L << 1;

void setDecorated(X11Context& ctx, Window window, bool decorated) {
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

void handleTitlebarButtonPress(X11Context& ctx, Window window,
                               const VeraHitTestRegions& regions,
                               int32_t clickX, int32_t clickY) {
    if (!regions.dragRegion ||
        !pointInRect(clickX, clickY, *regions.dragRegion))
        return;

    // Don't start a WM-driven move if the click actually landed on one of
    // the caption buttons drawn on top of the drag region.
    if (regions.minimizeButton &&
        pointInRect(clickX, clickY, *regions.minimizeButton))
        return;
    if (regions.maximizeButton &&
        pointInRect(clickX, clickY, *regions.maximizeButton))
        return;
    if (regions.closeButton &&
        pointInRect(clickX, clickY, *regions.closeButton))
        return;

    constexpr long MOVERESIZE_MOVE = 8;

    // _NET_WM_MOVERESIZE wants root-relative pointer coordinates.
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
    event.xclient.data.l[2] = MOVERESIZE_MOVE;
    event.xclient.data.l[3] = Button1;
    event.xclient.data.l[4] = 1;

    XUngrabPointer(ctx.display, CurrentTime);
    XSendEvent(ctx.display, ctx.root, False,
               SubstructureRedirectMask | SubstructureNotifyMask, &event);
}

}  // namespace vera::x11::window
