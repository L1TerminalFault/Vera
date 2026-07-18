#include "X11Keyboard.hxx"

#include <X11/Xlib.h>

#include "core/input/Keys.h"
#include "platform/x11/internal/X11XKB.hxx"

void handleKeyPressX11(
    X11Context& ctx, XKeyEvent& event, KeyStateArray& state,
    const std::function<void(VeraKey, bool, bool)>& keyCallback,
    const std::function<void(uint32_t)>& charCallback) {
    VeraKey key = convertKeycodeToVeraKeyX11(ctx, event.keycode);

    size_t idx = static_cast<size_t>(key);
    bool repeat = false;

    if (key != VeraKey::Unknown && idx < state.size()) {
        repeat = state[idx];

        if (!repeat) {
            state[idx] = true;

            KeyRepeatStateX11 repeatState{};
            repeatState.window = event.window;
            repeatState.key = event.keycode;
            repeatState.scanCode = event.keycode;
            repeatState.veraKey = key;
            repeatState.nextRepeat =
                std::chrono::steady_clock::now() +
                std::chrono::milliseconds(ctx.keyRepeatDelay);

            ctx.pressedKeys[event.keycode] = repeatState;
        }
    }

    // Ignore X11 auto-repeat events. Vera generates repeat events itself.
    if (repeat) {
        return;
    }

    if (keyCallback) {
        keyCallback(key, /*pressed=*/true, /*repeat=*/false);
    }

    if (charCallback) {
        uint32_t codepoint = convertKeyEventToCodepointX11(ctx, event);
        if (codepoint != 0) {
            charCallback(codepoint);
        }
    }
}

void handleKeyReleaseX11(
    X11Context& ctx, XKeyEvent& event, KeyStateArray& state,
    const std::function<void(VeraKey, bool, bool)>& keyCallback) {
    VeraKey key = convertKeycodeToVeraKeyX11(ctx, event.keycode);

    size_t idx = static_cast<size_t>(key);
    if (key != VeraKey::Unknown && idx < state.size()) {
        state[idx] = false;
    }

    ctx.pressedKeys.erase(event.keycode);

    if (keyCallback) {
        keyCallback(key, /*pressed=*/false, /*repeat=*/false);
    }
}
