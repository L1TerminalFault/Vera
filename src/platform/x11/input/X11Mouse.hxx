#pragma once

#include <X11/X.h>
#include <X11/Xlib.h>

#include <functional>

#include "core/input/Mouse.h"

static bool mapButton(unsigned int xButton, VeraMouseButton& out) {
    switch (xButton) {
        case Button1:
            out = VeraMouseButton::Left;
            return true;
        case Button2:
            out = VeraMouseButton::Middle;
            return true;
        case Button3:
            out = VeraMouseButton::Right;
            return true;
        case 8:
            out = VeraMouseButton::VeraButton4;
            return true;
        case 9:
            out = VeraMouseButton::VeraButton5;
            return true;
        default:
            return false;
    }
}

void handleButtonPress(
    XButtonEvent& event,
    const std::function<void(VeraMouseButton, bool)>& buttonCallback,
    const std::function<void(double, double)>& scrollCallback) {
    switch (event.button) {
        case Button4:
            if (scrollCallback) scrollCallback(0.0, 1.0);
            return;
        case Button5:
            if (scrollCallback) scrollCallback(0.0, -1.0);
            return;
        case 6:
            if (scrollCallback) scrollCallback(-1.0, 0.0);
            return;
        case 7:
            if (scrollCallback) scrollCallback(1.0, 0.0);
            return;
        default:
            break;
    }
    VeraMouseButton button;
    if (mapButton(event.button, button) && buttonCallback) {
        buttonCallback(button, true);
    }
}

void handleButtonRelease(
    XButtonEvent& event,
    const std::function<void(VeraMouseButton, bool)>& buttonCallback) {
    VeraMouseButton button;
    if (mapButton(event.button, button) && buttonCallback) {
        buttonCallback(button, false);
    }
}
