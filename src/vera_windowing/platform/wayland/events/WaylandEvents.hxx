#pragma once

#include "platform/wayland/internal/WaylandInternal.hxx"

void pollEventsWayland(WaylandContext& ctx,
                       const std::function<bool()>& quitRequestCallback,
                       const std::function<void()>& displayChangeCallback);

void waitForEventsWayland(WaylandContext& ctx,
                          const std::function<bool()>& quitRequestCallback,
                          const std::function<void()>& displayChangeCallback);

void waitForEventsWithTimeoutWayland(
    WaylandContext& ctx, double timeoutSeconds,
    const std::function<bool()>& quitRequestCallback,
    const std::function<void()>& displayChangeCallback);
