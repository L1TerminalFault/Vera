#pragma once

#include <poll.h>
#include <unistd.h>

#include <functional>
#include <iostream>

#include "platform/wayland/internal/WaylandInternal.hxx"

void poll(WaylandContext& ctx, const std::function<bool()>& quitRequestCallback,
          const std::function<void()>& displayChangeCallback) {
    (void)displayChangeCallback;

    if (!ctx.display) return;

    while (wl_display_prepare_read(ctx.display) != 0) {
        wl_display_dispatch_pending(ctx.display);
    }
    wl_display_flush(ctx.display);

    int fd = wl_display_get_fd(ctx.display);
    struct pollfd pfd = {fd, POLLIN, 0};

    int ret = ::poll(&pfd, 1, 0);

    if (ret > 0 && (pfd.revents & POLLIN)) {
        wl_display_read_events(ctx.display);
        wl_display_dispatch_pending(ctx.display);
    } else {
        wl_display_cancel_read(ctx.display);
    }

    if (ctx.quitRequested && quitRequestCallback) {
        if (quitRequestCallback()) {
            ctx.quitRequested = false;
        }
    }
}

void wait(WaylandContext& ctx, const std::function<bool()>& quitRequestCallback,
          const std::function<void()>& displayChangeCallback) {
    (void)displayChangeCallback;

    if (!ctx.display) return;

    wl_display_flush(ctx.display);

    if (wl_display_dispatch(ctx.display) == -1) {
        std::cerr << "[Wayland] Connection error on event dispatch loop.\n";
        ctx.quitRequested = true;
    }

    if (ctx.quitRequested && quitRequestCallback) {
        if (quitRequestCallback()) {
            ctx.quitRequested = false;
        }
    }
}

void waitTimeout(WaylandContext& ctx, double timeoutSeconds,
                 const std::function<bool()>& quitRequestCallback,
                 const std::function<void()>& displayChangeCallback) {
    (void)displayChangeCallback;

    if (!ctx.display) return;

    while (wl_display_prepare_read(ctx.display) != 0) {
        wl_display_dispatch_pending(ctx.display);
    }
    wl_display_flush(ctx.display);

    int fd = wl_display_get_fd(ctx.display);
    struct pollfd pfd = {fd, POLLIN, 0};

    int timeoutMs = static_cast<int>(timeoutSeconds * 1000.0);
    if (timeoutMs < 0) timeoutMs = -1;

    int ret = ::poll(&pfd, 1, timeoutMs);

    if (ret > 0 && (pfd.revents & POLLIN)) {
        wl_display_read_events(ctx.display);
        wl_display_dispatch_pending(ctx.display);
    } else {
        wl_display_cancel_read(ctx.display);
    }

    if (ctx.quitRequested && quitRequestCallback) {
        if (quitRequestCallback()) {
            ctx.quitRequested = false;
        }
    }
}
