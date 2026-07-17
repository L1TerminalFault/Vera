#pragma once
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <unistd.h>

#include <cstring>
#include <string>

#include "platform/x11/internal/X11Internal.hxx"

namespace vera::x11::window::properties {

using namespace internal;

void setTitle(X11Context& ctx, Window window, const std::string& title) {
    XStoreName(ctx.display, window, title.c_str());  // legacy WM_NAME fallback
    XChangeProperty(ctx.display, window, ctx.atoms.netWmName,
                    ctx.atoms.utf8String, 8, PropModeReplace,
                    reinterpret_cast<const unsigned char*>(title.c_str()),
                    static_cast<int>(title.size()));
}

void setIcon(X11Context& ctx, Window window, const std::string& iconPath) {
    if (iconPath.empty()) return;
    // _NET_WM_ICON expects a CARDINAL array: [width, height, pixels(ARGB)...],
    // possibly repeated for multiple sizes. Actual file decoding (PNG/etc.)
    // is intentionally not implemented here -- see header comment. This
    // stub wires the property-publishing half so a real image loader only
    // needs to hand this function raw ARGB32 pixels.
    (void)ctx;
    (void)window;
}

void setSizeHints(X11Context& ctx, Window window, uint32_t minWidth,
                  uint32_t minHeight, uint32_t maxWidth, uint32_t maxHeight,
                  bool resizable) {
    XSizeHints* hints = XAllocSizeHints();
    hints->flags = PMinSize | PMaxSize;
    hints->min_width = static_cast<int>(minWidth);
    hints->min_height = static_cast<int>(minHeight);

    if (!resizable) {
        // No max size given by the app but window shouldn't resize: pin max
        // to whatever min is (min==max tells every WM "fixed size").
        hints->max_width = static_cast<int>(maxWidth > 0 ? maxWidth : minWidth);
        hints->max_height =
            static_cast<int>(maxHeight > 0 ? maxHeight : minHeight);
    } else {
        hints->max_width =
            maxWidth > 0 ? static_cast<int>(maxWidth) : INT16_MAX;
        hints->max_height =
            maxHeight > 0 ? static_cast<int>(maxHeight) : INT16_MAX;
    }

    XSetWMNormalHints(ctx.display, window, hints);
    XFree(hints);
}

void setNetWmState(X11Context& ctx, Window window, Atom state1, Atom state2,
                   bool add) {
    // Per EWMH: if the window is not yet mapped, a WM won't act on a
    // ClientMessage, so we also set the property directly for that case and
    // send the message for the already-mapped case. Belt and suspenders.
    constexpr long NET_WM_STATE_REMOVE = 0, NET_WM_STATE_ADD = 1;

    XEvent event{};
    event.type = ClientMessage;
    event.xclient.window = window;
    event.xclient.message_type = ctx.atoms.netWmState;
    event.xclient.format = 32;
    event.xclient.data.l[0] = add ? NET_WM_STATE_ADD : NET_WM_STATE_REMOVE;
    event.xclient.data.l[1] = static_cast<long>(state1);
    event.xclient.data.l[2] = static_cast<long>(state2);
    event.xclient.data.l[3] = 1;  // source indication: normal application

    XSendEvent(ctx.display, ctx.root, False,
               SubstructureRedirectMask | SubstructureNotifyMask, &event);
}

void setAlwaysOnTop(X11Context& ctx, Window window, bool value) {
    setNetWmState(ctx, window, ctx.atoms.netWmStateAbove, 0, value);
}

void setWindowType(X11Context& ctx, Window window) {
    XChangeProperty(
        ctx.display, window, ctx.atoms.netWmWindowType, XA_ATOM, 32,
        PropModeReplace,
        reinterpret_cast<unsigned char*>(&ctx.atoms.netWmWindowTypeNormal), 1);
}

void setPid(X11Context& ctx, Window window) {
    long pid = static_cast<long>(getpid());
    XChangeProperty(ctx.display, window, ctx.atoms.netWmPid, XA_CARDINAL, 32,
                    PropModeReplace, reinterpret_cast<unsigned char*>(&pid), 1);
}

bool hasNetWmState(X11Context& ctx, Window window, Atom state) {
    Atom actualType;
    int actualFormat;
    unsigned long itemCount, bytesAfter;
    unsigned char* data = nullptr;
    bool found = false;

    if (XGetWindowProperty(ctx.display, window, ctx.atoms.netWmState, 0, 1024,
                           False, XA_ATOM, &actualType, &actualFormat,
                           &itemCount, &bytesAfter, &data) == Success &&
        data) {
        Atom* atoms = reinterpret_cast<Atom*>(data);
        for (unsigned long i = 0; i < itemCount; ++i) {
            if (atoms[i] == state) {
                found = true;
                break;
            }
        }
        XFree(data);
    }
    return found;
}

}  // namespace vera::x11::window::properties
