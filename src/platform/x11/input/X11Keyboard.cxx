#include "X11Keyboard.hxx"

#include <X11/Xlib.h>

#include <array>

#include "core/input/Keys.h"
#include "platform/x11/internal/X11XKB.hxx"

namespace vera::x11::input {

using VeraKey = internal::xkb::VeraKey;

using KeyStateArray = std::array<bool, static_cast<size_t>(VeraKey::Count)>;

void handleKeyPress(internal::X11Context& ctx, XKeyEvent& event,
                    KeyStateArray& state,
                    const std::function<void(VeraKey, bool, bool)>& keyCallback,
                    const std::function<void(uint32_t)>& charCallback) {
    VeraKey key = internal::xkb::keycodeToVeraKey(ctx, event.keycode);
    bool repeat = false;
    size_t idx = static_cast<size_t>(key);
    if (key != VeraKey::Unknown && idx < state.size()) {
        repeat = state[idx];
        state[idx] = true;
    }

    if (keyCallback) keyCallback(key, /*pressed=*/true, repeat);

    if (charCallback) {
        uint32_t codepoint = internal::xkb::keyEventToCodepoint(ctx, event);
        if (codepoint != 0) charCallback(codepoint);
    }
}

void handleKeyRelease(
    internal::X11Context& ctx, XKeyEvent& event, KeyStateArray& state,
    const std::function<void(VeraKey, bool, bool)>& keyCallback) {
    VeraKey key = internal::xkb::keycodeToVeraKey(ctx, event.keycode);
    size_t idx = static_cast<size_t>(key);
    if (key != VeraKey::Unknown && idx < state.size()) {
        state[idx] = false;
    }
    if (keyCallback) keyCallback(key, /*pressed=*/false, /*repeat=*/false);
}

}  // namespace vera::x11::input
