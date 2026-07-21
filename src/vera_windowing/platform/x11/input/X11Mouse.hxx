#pragma once

#include <X11/Xlib.h>

#include <functional>

#include "core/app/Types.h"

void handleMouseButtonPressX11(
    XButtonEvent& event,
    const std::function<void(VeraMouseButton, bool)>& buttonCallback,
    const std::function<void(double, double)>& scrollCallback);

void handleMouseButtonReleaseX11(
    XButtonEvent& event,
    const std::function<void(VeraMouseButton, bool)>& buttonCallback);
