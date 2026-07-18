#pragma once

#include <X11/Xlib.h>

#include <functional>

#include "core/input/Mouse.h"

void handleMouseButtonPressX11(
    XButtonEvent& event,
    const std::function<void(VeraMouseButton, bool)>& buttonCallback,
    const std::function<void(double, double)>& scrollCallback);

void handleMouseButtonReleaseX11(
    XButtonEvent& event,
    const std::function<void(VeraMouseButton, bool)>& buttonCallback);
