#pragma once

#include <functional>

#include "platform/x11/internal/X11Internal.hxx"

void pollEventsX11(X11Context& ctx,
                   const std::function<bool()>& quitRequestCallback,
                   const std::function<void()>& displayChangeCallback);

void waitForEventsX11(X11Context& ctx,
                      const std::function<bool()>& quitRequestCallback,
                      const std::function<void()>& displayChangeCallback);

void waitForEventsWithTimeoutX11(
    X11Context& ctx, double timeoutSeconds,
    const std::function<bool()>& quitRequestCallback,
    const std::function<void()>& displayChangeCallback);
