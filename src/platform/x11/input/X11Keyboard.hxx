#pragma once

#include <functional>

#include "core/input/Keys.h"
#include "platform/x11/internal/X11Internal.hxx"

using KeyStateArray = std::array<bool, static_cast<size_t>(VeraKey::Count)>;

void handleKeyPressX11(
    X11Context& ctx, XKeyEvent& event, KeyStateArray& state,
    const std::function<void(VeraKey, bool, bool)>& keyCallback,
    const std::function<void(uint32_t)>& charCallback);

void handleKeyReleaseX11(
    X11Context& ctx, XKeyEvent& event, KeyStateArray& state,
    const std::function<void(VeraKey, bool, bool)>& keyCallback);
