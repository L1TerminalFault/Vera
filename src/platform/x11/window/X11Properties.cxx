#include "X11Properties.hxx"

#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <unistd.h>

#include <cstdint>
#include <cstring>
#include <string>

void setTitleX11(X11Context& ctx, Window window, const std::string& title) {
    XStoreName(ctx.display, window, title.c_str());  // legacy WM_NAME fallback
    XChangeProperty(ctx.display, window, ctx.atoms.netWmName,
                    ctx.atoms.utf8String, 8, PropModeReplace,
                    reinterpret_cast<const unsigned char*>(title.c_str()),
                    static_cast<int>(title.size()));
}

void setIconX11(X11Context& ctx, Window window, const std::string& iconPath) {
    if (iconPath.empty()) return;
    (void)ctx;
    (void)window;
}

void setSizeHintsX11(X11Context& ctx, Window window, uint32_t minWidth,
                     uint32_t minHeight, uint32_t maxWidth, uint32_t maxHeight,
                     bool resizable) {
    XSizeHints* hints = XAllocSizeHints();
    hints->flags = PMinSize | PMaxSize;
    hints->min_width = static_cast<int>(minWidth);
    hints->min_height = static_cast<int>(minHeight);

    if (!resizable) {
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

void setNetWmStateX11(X11Context& ctx, Window window, Atom state1, Atom state2,
                      bool add) {
    constexpr int64_t netWmStateRemove = 0, netWmStateAdd = 1;

    XEvent event{};
    event.type = ClientMessage;
    event.xclient.window = window;
    event.xclient.message_type = ctx.atoms.netWmState;
    event.xclient.format = 32;
    event.xclient.data.l[0] = add ? netWmStateAdd : netWmStateRemove;
    event.xclient.data.l[1] = static_cast<int64_t>(state1);
    event.xclient.data.l[2] = static_cast<int64_t>(state2);
    event.xclient.data.l[3] = 1;  // source indication: normal application

    XSendEvent(ctx.display, ctx.root, False,
               SubstructureRedirectMask | SubstructureNotifyMask, &event);
}

void setAlwaysOnTopX11(X11Context& ctx, Window window, bool value) {
    setNetWmStateX11(ctx, window, ctx.atoms.netWmStateAbove, 0, value);
}

void setWindowTypeX11(X11Context& ctx, Window window) {
    XChangeProperty(
        ctx.display, window, ctx.atoms.netWmWindowType, XA_ATOM, 32,
        PropModeReplace,
        reinterpret_cast<unsigned char*>(&ctx.atoms.netWmWindowTypeNormal), 1);
}

void setPidX11(X11Context& ctx, Window window) {
    int64_t pid = static_cast<int64_t>(getpid());
    XChangeProperty(ctx.display, window, ctx.atoms.netWmPid, XA_CARDINAL, 32,
                    PropModeReplace, reinterpret_cast<unsigned char*>(&pid), 1);
}

bool hasNetWmStateX11(X11Context& ctx, Window window, Atom state) {
    Atom actualType;
    int actualFormat;
    ulong itemCount, bytesAfter;
    unsigned char* data = nullptr;
    bool found = false;

    if (XGetWindowProperty(ctx.display, window, ctx.atoms.netWmState, 0, 1024,
                           False, XA_ATOM, &actualType, &actualFormat,
                           &itemCount, &bytesAfter, &data) == Success &&
        data) {
        Atom* atoms = reinterpret_cast<Atom*>(data);
        for (ulong i = 0; i < itemCount; ++i) {
            if (atoms[i] == state) {
                found = true;
                break;
            }
        }
        XFree(data);
    }
    return found;
}
