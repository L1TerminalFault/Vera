#include "X11Keyboard.hxx"

#include <X11/Xlib.h>

#include "core/app/Types.h"
#include "platform/x11/internal/X11XKB.hxx"

void handleKeyPressX11(
    X11Context& ctx, XKeyEvent& event, KeyStateArray& state,
    const std::function<void(VeraKey, bool, bool)>& keyCallback,
    const std::function<void(uint32_t)>& charCallback) {
    // Pass the entire event object to match your updated modifier-aware
    // signature
    VeraKey key = convertKeyEventToVeraKeyX11(ctx, event);

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

    // Callback execution shifted AFTER state mapping synchronization to solve
    // the race bug
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
    VeraKey key = convertKeyEventToVeraKeyX11(ctx, event);

    // Fallback logic if layout states shifted during structural down/up
    // transitions
    auto mapIt = ctx.pressedKeys.find(event.keycode);
    if (mapIt != ctx.pressedKeys.end()) {
        VeraKey originalKey = mapIt->second.veraKey;
        if (originalKey != key) {
            // Unset tracking states for the old index to prevent stuck input
            // loops
            size_t origIdx = static_cast<size_t>(originalKey);
            if (origIdx < state.size()) {
                state[origIdx] = false;
            }
            key = originalKey;  // Align final callback signatures with original
                                // click data
        }
        ctx.pressedKeys.erase(mapIt);
    } else {
        // Fallback for untracked corner cases
        size_t idx = static_cast<size_t>(key);
        if (key != VeraKey::Unknown && idx < state.size()) {
            state[idx] = false;
        }
    }

    if (keyCallback) {
        keyCallback(key, /*pressed=*/false, /*repeat=*/false);
    }
}
