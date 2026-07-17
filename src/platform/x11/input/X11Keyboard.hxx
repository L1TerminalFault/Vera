#pragma once

#include <functional>

#include "platform/x11/internal/X11XKB.hxx"

namespace vera::x11::input {

using KeyStateArray =
    std::array<bool, static_cast<size_t>(core::input::VeraKey::Count)>;

void handleKeyPress(
    internal::X11Context& ctx, XKeyEvent& event, KeyStateArray& state,
    const std::function<void(core::input::VeraKey, bool, bool)>& keyCallback,
    const std::function<void(uint32_t)>& charCallback);

void handleKeyRelease(
    internal::X11Context& ctx, XKeyEvent& event, KeyStateArray& state,
    const std::function<void(core::input::VeraKey, bool, bool)>& keyCallback);

}  // namespace vera::x11::input
