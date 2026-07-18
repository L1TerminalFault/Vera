#include "X11Events.hxx"

#include <X11/extensions/Xrandr.h>
#include <sys/select.h>

#include <cstdint>
#include <functional>

#include "platform/x11/desktop/X11Clipboard.hxx"
#include "platform/x11/desktop/X11DragDrop.hxx"
#include "platform/x11/desktop/X11Theme.hxx"
#include "platform/x11/window/X11Window.hxx"

static int gRandrEventBase = 0;

static void dispatchOne(X11Context& ctx, XEvent& event,
                        const std::function<bool()>& quitRequestCallback,
                        const std::function<void()>& displayChangeCallback) {
    if (gRandrEventBase &&
        event.type == gRandrEventBase + RRScreenChangeNotify) {
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
            handleClientMessage(ctx, window, event.xclient);
            return;
        }
        case SelectionRequest:
            handleClipboardSelectionRequestX11(ctx, event.xselectionrequest);
            return;
        case SelectionClear:
            handleClipboardSelectionClearX11(ctx, event.xselectionclear);
            return;
        case SelectionNotify: {
            for (auto& [xid, window] : ctx.windowsByXid) {
                if (static_cast<Window>(window->getNativeHandle().x11Window) ==
                    event.xselection.requestor) {
                    handleSelectionNotify(ctx, window, event.xselection);
                    break;
                }
            }
            return;
        }
        case PropertyNotify:
            handleThemePropertyNotifyX11(ctx, event.xproperty);
            [[fallthrough]];
        default: {
            auto it = ctx.windowsByXid.find(event.xany.window);
            if (it != ctx.windowsByXid.end()) it->second->handleXEvent(event);
        }
    }
}

void pollEventsX11(X11Context& ctx,
                   const std::function<bool()>& quitRequestCallback,
                   const std::function<void()>& displayChangeCallback) {
    if (!gRandrEventBase) {
        int errorBase;
        XRRQueryExtension(ctx.display, &gRandrEventBase, &errorBase);
    }
    while (XPending(ctx.display) > 0) {
        XEvent event;
        XNextEvent(ctx.display, &event);
        dispatchOne(ctx, event, quitRequestCallback, displayChangeCallback);
    }
}

void waitForEventsX11(X11Context& ctx,
                      const std::function<bool()>& quitRequestCallback,
                      const std::function<void()>& displayChangeCallback) {
    XEvent event;
    XNextEvent(ctx.display, &event);
    dispatchOne(ctx, event, quitRequestCallback, displayChangeCallback);
    pollEventsX11(ctx, quitRequestCallback, displayChangeCallback);
}

void waitForEventsWithTimeoutX11(
    X11Context& ctx, double timeoutSeconds,
    const std::function<bool()>& quitRequestCallback,
    const std::function<void()>& displayChangeCallback) {
    if (XPending(ctx.display) > 0) {
        pollEventsX11(ctx, quitRequestCallback, displayChangeCallback);
        return;
    }

    int fd = ConnectionNumber(ctx.display);
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(fd, &fds);
    timeval tv;
    tv.tv_sec = static_cast<int64_t>(timeoutSeconds);
    tv.tv_usec = static_cast<int64_t>((timeoutSeconds - tv.tv_sec) * 1'000'000);

    if (select(fd + 1, &fds, nullptr, nullptr, &tv) > 0) {
        pollEventsX11(ctx, quitRequestCallback, displayChangeCallback);
    }
}
