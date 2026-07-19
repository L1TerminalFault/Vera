#include "X11Events.hxx"

#include <X11/extensions/Xrandr.h>
#include <sys/select.h>
#include <unistd.h>

#include <chrono>
#include <cstdint>
#include <functional>

#include "platform/x11/desktop/X11Clipboard.hxx"
#include "platform/x11/desktop/X11DragDrop.hxx"
#include "platform/x11/desktop/X11Theme.hxx"
#include "platform/x11/input/X11Joystick.hxx"
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

static void processKeyRepeat(X11Context* ctx) {
    if (ctx->keyRepeatRate == 0 || ctx->pressedKeys.empty()) {
        return;
    }

    auto now = std::chrono::steady_clock::now();

    for (auto& [key, repeatState] : ctx->pressedKeys) {
        if (now >= repeatState.nextRepeat) {
            auto it = ctx->windowsByXid.find(repeatState.window);
            if (it != ctx->windowsByXid.end()) {
                auto* window = it->second;

                if (window && window->getKeyCallback()) {
                    window->getKeyCallback()(repeatState.veraKey, true, true);
                }
            }

            repeatState.nextRepeat =
                now + std::chrono::milliseconds(1000 / ctx->keyRepeatRate);
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

    // Read non-blocking input updates from controller drivers
    updateJoystickX11(ctx);

    while (XPending(ctx.display) > 0) {
        XEvent event;
        XNextEvent(ctx.display, &event);
        dispatchOne(ctx, event, quitRequestCallback, displayChangeCallback);
    }

    processKeyRepeat(&ctx);
}

void waitForEventsX11(X11Context& ctx,
                      const std::function<bool()>& quitRequestCallback,
                      const std::function<void()>& displayChangeCallback) {
    if (XPending(ctx.display) > 0) {
        pollEventsX11(ctx, quitRequestCallback, displayChangeCallback);
        return;
    }

    // Direct fallback into our multi-device select handler using a safe 10ms
    // responsive step
    waitForEventsWithTimeoutX11(ctx, 0.010, quitRequestCallback,
                                displayChangeCallback);
}

void waitForEventsWithTimeoutX11(
    X11Context& ctx, double timeoutSeconds,
    const std::function<bool()>& quitRequestCallback,
    const std::function<void()>& displayChangeCallback) {
    // Refresh joystick state maps before evaluating thread blocks
    updateJoystickX11(ctx);

    if (XPending(ctx.display) > 0) {
        pollEventsX11(ctx, quitRequestCallback, displayChangeCallback);
        return;
    }

    int x11Fd = ConnectionNumber(ctx.display);
    int maxFd = x11Fd;

    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(x11Fd, &fds);

    // Multiplex all active context joystick file descriptors directly into
    // select
    for (const auto& joy : ctx.joysticks) {
        if (joy.fd >= 0) {
            FD_SET(joy.fd, &fds);
            if (joy.fd > maxFd) {
                maxFd = joy.fd;
            }
        }
    }

    timeval tv;
    tv.tv_sec = static_cast<int64_t>(timeoutSeconds);
    tv.tv_usec = static_cast<int64_t>((timeoutSeconds - tv.tv_sec) * 1'000'000);

    // Thread wakes immediately when X11 messages arrive OR joystick events fire
    if (select(maxFd + 1, &fds, nullptr, nullptr, &tv) > 0) {
        pollEventsX11(ctx, quitRequestCallback, displayChangeCallback);
    } else {
        // Fallback update to process joystick disconnection rules and edge
        // clocks on timeouts
        updateJoystickX11(ctx);
        processKeyRepeat(&ctx);
    }
}
