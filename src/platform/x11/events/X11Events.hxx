#pragma once
#include <X11/extensions/Xrandr.h>
#include <sys/select.h>

#include <functional>

#include "platform/x11/desktop/X11Clipboard.hxx"
#include "platform/x11/desktop/X11DragDrop.hxx"
#include "platform/x11/desktop/X11Theme.hxx"
#include "platform/x11/internal/X11Internal.hxx"
#include "platform/x11/window/X11Window.hxx"

namespace vera::x11::events {

using namespace desktop;
using namespace internal;

static int g_randrEventBase = 0;

static void dispatchOne(X11Context& ctx, XEvent& event,
                        const std::function<bool()>& quitRequestCallback,
                        const std::function<void()>& displayChangeCallback) {
    // RandR hotplug/resolution-change notifications arrive on a base offset
    // rather than a fixed event type, so they're checked first.
    if (g_randrEventBase &&
        event.type == g_randrEventBase + RRScreenChangeNotify) {
        XRRUpdateConfiguration(&event);
        if (displayChangeCallback) displayChangeCallback();
        return;
    }

    switch (event.type) {
        case ClientMessage: {
            auto it = ctx.windowsByXid.find(event.xclient.window);
            if (it == ctx.windowsByXid.end()) return;
            X11Window* window = it->second;

            if (event.xclient.message_type == ctx.atoms.wmProtocols &&
                static_cast<Atom>(event.xclient.data.l[0]) ==
                    ctx.atoms.wmDeleteWindow) {
                bool allowQuit = true;
                if (quitRequestCallback) {
                    allowQuit = quitRequestCallback();
                }

                if (allowQuit) {
                    window->handleWmCloseRequest();
                }
                return;
            }
            dnd::handleClientMessage(ctx, window, event.xclient);
            return;
        }
        case SelectionRequest:
            clipboard::handleSelectionRequest(ctx, event.xselectionrequest);
            return;
        case SelectionClear:
            clipboard::handleSelectionClear(ctx, event.xselectionclear);
            return;
        case SelectionNotify: {
            // Could be a clipboard transfer (handled synchronously in
            // X11Clipboard::getText via XCheckTypedWindowEvent) or an XDND
            // file-list payload -- route by requestor window.
            for (auto& [xid, window] : ctx.windowsByXid) {
                if (static_cast<Window>(window->getNativeHandle().x11Window) ==
                    event.xselection.requestor) {
                    dnd::handleSelectionNotify(ctx, window, event.xselection);
                    break;
                }
            }
            return;
        }
        case PropertyNotify:
            theme::handlePropertyNotify(ctx, event.xproperty);
            [[fallthrough]];
        default: {
            auto it = ctx.windowsByXid.find(event.xany.window);
            if (it != ctx.windowsByXid.end()) it->second->handleXEvent(event);
        }
    }
}

void poll(X11Context& ctx, const std::function<bool()>& quitRequestCallback,
          const std::function<void()>& displayChangeCallback) {
    if (!g_randrEventBase) {
        int errorBase;
        XRRQueryExtension(ctx.display, &g_randrEventBase, &errorBase);
    }
    while (XPending(ctx.display) > 0) {
        XEvent event;
        XNextEvent(ctx.display, &event);
        dispatchOne(ctx, event, quitRequestCallback, displayChangeCallback);
    }
}

void wait(X11Context& ctx, const std::function<bool()>& quitRequestCallback,
          const std::function<void()>& displayChangeCallback) {
    XEvent event;
    XNextEvent(ctx.display, &event);  // blocks until at least one event
    dispatchOne(ctx, event, quitRequestCallback, displayChangeCallback);
    poll(ctx, quitRequestCallback, displayChangeCallback);  // drain the rest
}

void waitTimeout(X11Context& ctx, double timeoutSeconds,
                 const std::function<bool()>& quitRequestCallback,
                 const std::function<void()>& displayChangeCallback) {
    if (XPending(ctx.display) > 0) {
        poll(ctx, quitRequestCallback, displayChangeCallback);
        return;
    }

    int fd = ConnectionNumber(ctx.display);
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(fd, &fds);
    timeval tv;
    tv.tv_sec = static_cast<long>(timeoutSeconds);
    tv.tv_usec = static_cast<long>((timeoutSeconds - tv.tv_sec) * 1'000'000);

    if (select(fd + 1, &fds, nullptr, nullptr, &tv) > 0) {
        poll(ctx, quitRequestCallback, displayChangeCallback);
    }
}

}  // namespace vera::x11::events
