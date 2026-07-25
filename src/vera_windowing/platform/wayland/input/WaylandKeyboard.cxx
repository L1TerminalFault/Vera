#include "WaylandKeyboard.hxx"

#include <sys/mman.h>
#include <unistd.h>
#include <wayland-client-protocol.h>
#include <xkbcommon/xkbcommon.h>

#include <chrono>
#include <cstdint>

#include "platform/wayland/internal/WaylandInternal.hxx"
#include "platform/wayland/window/WaylandWindow.hxx"

static VeraKey translateKeysym(xkb_keysym_t sym) {
    switch (sym) {
        // Lowercase Letters
        case XKB_KEY_a:
            return VeraKey::ALower;
        case XKB_KEY_b:
            return VeraKey::BLower;
        case XKB_KEY_c:
            return VeraKey::CLower;
        case XKB_KEY_d:
            return VeraKey::DLower;
        case XKB_KEY_e:
            return VeraKey::ELower;
        case XKB_KEY_f:
            return VeraKey::FLower;
        case XKB_KEY_g:
            return VeraKey::GLower;
        case XKB_KEY_h:
            return VeraKey::HLower;
        case XKB_KEY_i:
            return VeraKey::ILower;
        case XKB_KEY_j:
            return VeraKey::JLower;
        case XKB_KEY_k:
            return VeraKey::KLower;
        case XKB_KEY_l:
            return VeraKey::LLower;
        case XKB_KEY_m:
            return VeraKey::MLower;
        case XKB_KEY_n:
            return VeraKey::NLower;
        case XKB_KEY_o:
            return VeraKey::OLower;
        case XKB_KEY_p:
            return VeraKey::PLower;
        case XKB_KEY_q:
            return VeraKey::QLower;
        case XKB_KEY_r:
            return VeraKey::RLower;
        case XKB_KEY_s:
            return VeraKey::SLower;
        case XKB_KEY_t:
            return VeraKey::TLower;
        case XKB_KEY_u:
            return VeraKey::ULower;
        case XKB_KEY_v:
            return VeraKey::VLower;
        case XKB_KEY_w:
            return VeraKey::WLower;
        case XKB_KEY_x:
            return VeraKey::XLower;
        case XKB_KEY_y:
            return VeraKey::YLower;
        case XKB_KEY_z:
            return VeraKey::ZLower;

        // Uppercase LetterLowers
        case XKB_KEY_A:
            return VeraKey::AUpper;
        case XKB_KEY_B:
            return VeraKey::BUpper;
        case XKB_KEY_C:
            return VeraKey::CUpper;
        case XKB_KEY_D:
            return VeraKey::DUpper;
        case XKB_KEY_E:
            return VeraKey::EUpper;
        case XKB_KEY_F:
            return VeraKey::FUpper;
        case XKB_KEY_G:
            return VeraKey::GUpper;
        case XKB_KEY_H:
            return VeraKey::HUpper;
        case XKB_KEY_I:
            return VeraKey::IUpper;
        case XKB_KEY_J:
            return VeraKey::JUpper;
        case XKB_KEY_K:
            return VeraKey::KUpper;
        case XKB_KEY_L:
            return VeraKey::LUpper;
        case XKB_KEY_M:
            return VeraKey::MUpper;
        case XKB_KEY_N:
            return VeraKey::NUpper;
        case XKB_KEY_O:
            return VeraKey::OUpper;
        case XKB_KEY_P:
            return VeraKey::PUpper;
        case XKB_KEY_Q:
            return VeraKey::QUpper;
        case XKB_KEY_R:
            return VeraKey::RUpper;
        case XKB_KEY_S:
            return VeraKey::SUpper;
        case XKB_KEY_T:
            return VeraKey::TUpper;
        case XKB_KEY_U:
            return VeraKey::UUpper;
        case XKB_KEY_V:
            return VeraKey::VUpper;
        case XKB_KEY_W:
            return VeraKey::WUpper;
        case XKB_KEY_X:
            return VeraKey::XUpper;
        case XKB_KEY_Y:
            return VeraKey::YUpper;
        case XKB_KEY_Z:
            return VeraKey::ZUpper;

        // Numbers
        case XKB_KEY_0:
            return VeraKey::Num0;
        case XKB_KEY_1:
            return VeraKey::Num1;
        case XKB_KEY_2:
            return VeraKey::Num2;
        case XKB_KEY_3:
            return VeraKey::Num3;
        case XKB_KEY_4:
            return VeraKey::Num4;
        case XKB_KEY_5:
            return VeraKey::Num5;
        case XKB_KEY_6:
            return VeraKey::Num6;
        case XKB_KEY_7:
            return VeraKey::Num7;
        case XKB_KEY_8:
            return VeraKey::Num8;
        case XKB_KEY_9:
            return VeraKey::Num9;

        // Shifted Symbols
        case XKB_KEY_exclam:
            return VeraKey::Exclamation;
        case XKB_KEY_at:
            return VeraKey::At;
        case XKB_KEY_numbersign:
            return VeraKey::Hash;
        case XKB_KEY_dollar:
            return VeraKey::Dollar;
        case XKB_KEY_percent:
            return VeraKey::Percent;
        case XKB_KEY_asciicircum:
            return VeraKey::Caret;
        case XKB_KEY_ampersand:
            return VeraKey::Ampersand;
        case XKB_KEY_asterisk:
            return VeraKey::Asterisk;
        case XKB_KEY_parenleft:
            return VeraKey::LeftParen;
        case XKB_KEY_parenright:
            return VeraKey::RightParen;
        case XKB_KEY_underscore:
            return VeraKey::Underscore;
        case XKB_KEY_plus:
            return VeraKey::Plus;
        case XKB_KEY_colon:
            return VeraKey::Colon;
        case XKB_KEY_quotedbl:
            return VeraKey::Quote;
        case XKB_KEY_less:
            return VeraKey::LessThan;
        case XKB_KEY_greater:
            return VeraKey::GreaterThan;
        case XKB_KEY_question:
            return VeraKey::Question;
        case XKB_KEY_braceleft:
            return VeraKey::LeftBrace;
        case XKB_KEY_braceright:
            return VeraKey::RightBrace;
        case XKB_KEY_bar:
            return VeraKey::Pipe;
        case XKB_KEY_asciitilde:
            return VeraKey::Tilde;

        // Base Symbols
        case XKB_KEY_space:
            return VeraKey::Space;
        case XKB_KEY_apostrophe:
            return VeraKey::Apostrophe;
        case XKB_KEY_comma:
            return VeraKey::Comma;
        case XKB_KEY_minus:
            return VeraKey::Minus;
        case XKB_KEY_period:
            return VeraKey::Period;
        case XKB_KEY_slash:
            return VeraKey::Slash;
        case XKB_KEY_semicolon:
            return VeraKey::Semicolon;
        case XKB_KEY_equal:
            return VeraKey::Equal;
        case XKB_KEY_bracketleft:
            return VeraKey::LeftBracket;
        case XKB_KEY_backslash:
            return VeraKey::Backslash;
        case XKB_KEY_bracketright:
            return VeraKey::RightBracket;
        case XKB_KEY_grave:
            return VeraKey::GraveAccent;

        // System Control
        case XKB_KEY_Return:
            return VeraKey::Enter;
        case XKB_KEY_Escape:
            return VeraKey::Escape;
        case XKB_KEY_Tab:
            return VeraKey::Tab;
        case XKB_KEY_BackSpace:
            return VeraKey::Backspace;
        case XKB_KEY_Insert:
            return VeraKey::Insert;
        case XKB_KEY_Delete:
            return VeraKey::Delete;
        case XKB_KEY_Home:
            return VeraKey::Home;
        case XKB_KEY_End:
            return VeraKey::End;
        case XKB_KEY_Prior:
            return VeraKey::PageUp;
        case XKB_KEY_Next:
            return VeraKey::PageDown;
        case XKB_KEY_Left:
            return VeraKey::Left;
        case XKB_KEY_Right:
            return VeraKey::Right;
        case XKB_KEY_Up:
            return VeraKey::Up;
        case XKB_KEY_Down:
            return VeraKey::Down;

        // Modifiers
        case XKB_KEY_Shift_L:
            return VeraKey::LeftShift;
        case XKB_KEY_Shift_R:
            return VeraKey::RightShift;
        case XKB_KEY_Control_L:
            return VeraKey::LeftCtrl;
        case XKB_KEY_Control_R:
            return VeraKey::RightCtrl;
        case XKB_KEY_Alt_L:
            return VeraKey::LeftAlt;
        case XKB_KEY_Alt_R:
            return VeraKey::RightAlt;
        case XKB_KEY_Super_L:
            return VeraKey::LeftSuper;
        case XKB_KEY_Super_R:
            return VeraKey::RightSuper;

        // Locks & Utility
        case XKB_KEY_Caps_Lock:
            return VeraKey::CapsLock;
        case XKB_KEY_Scroll_Lock:
            return VeraKey::ScrollLock;
        case XKB_KEY_Num_Lock:
            return VeraKey::NumLock;
        case XKB_KEY_Print:
            return VeraKey::PrintScreen;
        case XKB_KEY_Pause:
            return VeraKey::Pause;
        case XKB_KEY_Menu:
            return VeraKey::Menu;

        // Function Keys
        case XKB_KEY_F1:
            return VeraKey::F1;
        case XKB_KEY_F2:
            return VeraKey::F2;
        case XKB_KEY_F3:
            return VeraKey::F3;
        case XKB_KEY_F4:
            return VeraKey::F4;
        case XKB_KEY_F5:
            return VeraKey::F5;
        case XKB_KEY_F6:
            return VeraKey::F6;
        case XKB_KEY_F7:
            return VeraKey::F7;
        case XKB_KEY_F8:
            return VeraKey::F8;
        case XKB_KEY_F9:
            return VeraKey::F9;
        case XKB_KEY_F10:
            return VeraKey::F10;
        case XKB_KEY_F11:
            return VeraKey::F11;
        case XKB_KEY_F12:
            return VeraKey::F12;

        // Keypad
        case XKB_KEY_KP_0:
            return VeraKey::KP0;
        case XKB_KEY_KP_1:
            return VeraKey::KP1;
        case XKB_KEY_KP_2:
            return VeraKey::KP2;
        case XKB_KEY_KP_3:
            return VeraKey::KP3;
        case XKB_KEY_KP_4:
            return VeraKey::KP4;
        case XKB_KEY_KP_5:
            return VeraKey::KP5;
        case XKB_KEY_KP_6:
            return VeraKey::KP6;
        case XKB_KEY_KP_7:
            return VeraKey::KP7;
        case XKB_KEY_KP_8:
            return VeraKey::KP8;
        case XKB_KEY_KP_9:
            return VeraKey::KP9;
        case XKB_KEY_KP_Decimal:
            return VeraKey::KPDecimal;
        case XKB_KEY_KP_Divide:
            return VeraKey::KPDivide;
        case XKB_KEY_KP_Multiply:
            return VeraKey::KPMultiply;
        case XKB_KEY_KP_Subtract:
            return VeraKey::KPSubtract;
        case XKB_KEY_KP_Add:
            return VeraKey::KPAdd;
        case XKB_KEY_KP_Enter:
            return VeraKey::KPEnter;
        case XKB_KEY_KP_Equal:
            return VeraKey::KPEqual;

        // Audio
        case XKB_KEY_XF86AudioRaiseVolume:
            return VeraKey::VolumeUp;
        case XKB_KEY_XF86AudioLowerVolume:
            return VeraKey::VolumeDown;
        case XKB_KEY_XF86AudioMute:
            return VeraKey::Mute;

        default:
            return VeraKey::Unknown;
    }
}

static void keyboardHandleKeymap(void* data, wl_keyboard* keyboard,
                                 uint32_t format, int32_t fd, uint32_t size) {
    auto* ctx = static_cast<WaylandContext*>(data);
    (void)keyboard;

    if (format != WL_KEYBOARD_KEYMAP_FORMAT_XKB_V1) {
        close(fd);
        return;
    }

    char* mapStr =
        static_cast<char*>(mmap(nullptr, size, PROT_READ, MAP_SHARED, fd, 0));
    if (mapStr == MAP_FAILED) {
        close(fd);
        return;
    }

    if (!ctx->xkbContext) {
        ctx->xkbContext = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
    }

    if (ctx->xkbKeymap) xkb_keymap_unref(ctx->xkbKeymap);
    ctx->xkbKeymap = xkb_keymap_new_from_string(ctx->xkbContext, mapStr,
                                                XKB_KEYMAP_FORMAT_TEXT_V1,
                                                XKB_KEYMAP_COMPILE_NO_FLAGS);
    munmap(mapStr, size);
    close(fd);

    if (ctx->xkbKeymap) {
        if (ctx->xkbState) xkb_state_unref(ctx->xkbState);
        ctx->xkbState = xkb_state_new(ctx->xkbKeymap);
    }
}

static void keyboardHandleEnter(void* data, wl_keyboard* keyboard,
                                uint32_t serial, wl_surface* surface,
                                wl_array* keys) {
    auto* ctx = static_cast<WaylandContext*>(data);
    (void)keyboard;
    (void)serial;
    (void)keys;

    auto it = ctx->windowsBySurface.find(surface);
    if (it != ctx->windowsBySurface.end()) {
        ctx->focusedWindow = it->second;
    }
}

static void keyboardHandleLeave(void* data, wl_keyboard* keyboard,
                                uint32_t serial, wl_surface* surface) {
    auto* ctx = static_cast<WaylandContext*>(data);
    (void)keyboard;
    (void)serial;
    (void)surface;
    ctx->focusedWindow = nullptr;
}

static void keyboardHandleKey(void* data, wl_keyboard* keyboard,
                              uint32_t serial, uint32_t time, uint32_t key,
                              uint32_t state) {
    auto* ctx = static_cast<WaylandContext*>(data);
    (void)keyboard;
    (void)serial;
    (void)time;

    if (!ctx->focusedWindow || !ctx->xkbState) return;

    auto* win = ctx->focusedWindow;
    uint32_t scanCode = key + 8;

    // xkb common automatically evaluates Shift/Caps modifiers into the matching
    // sym
    xkb_keysym_t sym = xkb_state_key_get_one_sym(ctx->xkbState, scanCode);
    VeraKey veraKey = translateKeysym(sym);
    bool pressed = (state == WL_KEYBOARD_KEY_STATE_PRESSED);

    // FIX #1: Update the central state tracking array BEFORE firing callbacks!
    // This allows window->isPressed queries to work correctly inside the
    // callback scope.
    if (veraKey != VeraKey::Unknown && veraKey != VeraKey::Count) {
        size_t keyIndex = static_cast<size_t>(veraKey);
        if (keyIndex < std::size(ctx->keyStates)) {
            ctx->keyStates[keyIndex] = pressed;
        }
    }

    // FIX #2: Handle case logic where releasing a key might have a different
    // active layout sym than when it was pressed (e.g., pressing Shift + A,
    // then releasing Shift, then releasing A)
    if (pressed) {
        KeyRepeatStateWayland repeatState{};
        repeatState.key = key;
        repeatState.veraKey = veraKey;
        repeatState.scanCode = scanCode;

        auto now = std::chrono::steady_clock::now();
        repeatState.nextRepeat =
            now + std::chrono::milliseconds(ctx->keyRepeatDelay);

        ctx->pressedKeys[key] = repeatState;
    } else {
        // If the keysym shifted on release, fallback to what we recorded during
        // the press
        auto it = ctx->pressedKeys.find(key);
        if (it != ctx->pressedKeys.end()) {
            VeraKey originalKey = it->second.veraKey;
            if (originalKey != veraKey) {
                // Clear the state of the original tracked key too
                size_t origIndex = static_cast<size_t>(originalKey);
                if (origIndex < std::size(ctx->keyStates)) {
                    ctx->keyStates[origIndex] = false;
                }
                veraKey = originalKey;  // Deliver callback matching the
                                        // original press
            }
            ctx->pressedKeys.erase(it);
        }
    }

    // Fire the application callback now that contextual array states match
    // reality
    if (win->getKeyCallback()) {
        win->getKeyCallback()(veraKey, pressed, false);
    }

    // Process Text Input Engines
    if (pressed && win->getCharCallback()) {
        uint32_t utf32 = xkb_state_key_get_utf32(ctx->xkbState, scanCode);
        if (utf32 > 0) {
            win->getCharCallback()(utf32);
        }
    }
}

static void keyboardHandleModifiers(void* data, wl_keyboard* keyboard,
                                    uint32_t serial, uint32_t mods_depressed,
                                    uint32_t mods_latched, uint32_t mods_locked,
                                    uint32_t group) {
    auto* ctx = static_cast<WaylandContext*>(data);
    (void)keyboard;
    (void)serial;

    if (ctx->xkbState) {
        xkb_state_update_mask(ctx->xkbState, mods_depressed, mods_latched,
                              mods_locked, 0, 0, group);
    }
}

static void keyboardHandleRepeatInfo(void* data, wl_keyboard* keyboard,
                                     int32_t rate, int32_t delay) {
    auto* ctx = static_cast<WaylandContext*>(data);

    (void)keyboard;
    ctx->keyRepeatRate = ctx->keyRepeatRate ? ctx->keyRepeatRate : rate;
    ctx->keyRepeatDelay = ctx->keyRepeatDelay ? ctx->keyRepeatDelay : delay;
}

const wl_keyboard_listener KKEYBOARD_LISTENER = {
    .keymap = keyboardHandleKeymap,
    .enter = keyboardHandleEnter,
    .leave = keyboardHandleLeave,
    .key = keyboardHandleKey,
    .modifiers = keyboardHandleModifiers,
    .repeat_info = keyboardHandleRepeatInfo};

void addListenerToKeyboard(WaylandContext& ctx, wl_keyboard* keyboard) {
    if (keyboard) {
        wl_keyboard_add_listener(keyboard, &KKEYBOARD_LISTENER, &ctx);
    }
}
