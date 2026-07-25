#include "X11XKB.hxx"

#include <X11/XF86keysym.h>
#include <X11/XKBlib.h>
#include <X11/Xlib.h>

#include <array>
#include <cstdint>

#include "core/app/Types.h"
#include "platform/x11/internal/X11Internal.hxx"
#include "platform/x11/window/X11Window.hxx"

void initializeXKBX11(X11Context& ctx) {
    int major = XkbMajorVersion, minor = XkbMinorVersion;
    XkbLibraryVersion(&major, &minor);
    XkbQueryExtension(ctx.display, nullptr, nullptr, nullptr, &major, &minor);

    Bool supported = False;
    XkbSetDetectableAutoRepeat(ctx.display, True, &supported);
}

static VeraKey mapKeysym(KeySym ks) {
    switch (ks) {
        // Lowercase Letters
        case XK_a:
            return VeraKey::ALower;
        case XK_b:
            return VeraKey::BLower;
        case XK_c:
            return VeraKey::CLower;
        case XK_d:
            return VeraKey::DLower;
        case XK_e:
            return VeraKey::ELower;
        case XK_f:
            return VeraKey::FLower;
        case XK_g:
            return VeraKey::GLower;
        case XK_h:
            return VeraKey::HLower;
        case XK_i:
            return VeraKey::ILower;
        case XK_j:
            return VeraKey::JLower;
        case XK_k:
            return VeraKey::KLower;
        case XK_l:
            return VeraKey::LLower;
        case XK_m:
            return VeraKey::MLower;
        case XK_n:
            return VeraKey::NLower;
        case XK_o:
            return VeraKey::OLower;
        case XK_p:
            return VeraKey::PLower;
        case XK_q:
            return VeraKey::QLower;
        case XK_r:
            return VeraKey::RLower;
        case XK_s:
            return VeraKey::SLower;
        case XK_t:
            return VeraKey::TLower;
        case XK_u:
            return VeraKey::ULower;
        case XK_v:
            return VeraKey::VLower;
        case XK_w:
            return VeraKey::WLower;
        case XK_x:
            return VeraKey::XLower;
        case XK_y:
            return VeraKey::YLower;
        case XK_z:
            return VeraKey::ZLower;

        // Uppercase Letters
        case XK_A:
            return VeraKey::AUpper;
        case XK_B:
            return VeraKey::BUpper;
        case XK_C:
            return VeraKey::CUpper;
        case XK_D:
            return VeraKey::DUpper;
        case XK_E:
            return VeraKey::EUpper;
        case XK_F:
            return VeraKey::FUpper;
        case XK_G:
            return VeraKey::GUpper;
        case XK_H:
            return VeraKey::HUpper;
        case XK_I:
            return VeraKey::IUpper;
        case XK_J:
            return VeraKey::JUpper;
        case XK_K:
            return VeraKey::KUpper;
        case XK_L:
            return VeraKey::LUpper;
        case XK_M:
            return VeraKey::MUpper;
        case XK_N:
            return VeraKey::NUpper;
        case XK_O:
            return VeraKey::OUpper;
        case XK_P:
            return VeraKey::PUpper;
        case XK_Q:
            return VeraKey::QUpper;
        case XK_R:
            return VeraKey::RUpper;
        case XK_S:
            return VeraKey::SUpper;
        case XK_T:
            return VeraKey::TUpper;
        case XK_U:
            return VeraKey::UUpper;
        case XK_V:
            return VeraKey::VUpper;
        case XK_W:
            return VeraKey::WUpper;
        case XK_X:
            return VeraKey::XUpper;
        case XK_Y:
            return VeraKey::YUpper;
        case XK_Z:
            return VeraKey::ZUpper;

        // Base Numbers
        case XK_0:
            return VeraKey::Num0;
        case XK_1:
            return VeraKey::Num1;
        case XK_2:
            return VeraKey::Num2;
        case XK_3:
            return VeraKey::Num3;
        case XK_4:
            return VeraKey::Num4;
        case XK_5:
            return VeraKey::Num5;
        case XK_6:
            return VeraKey::Num6;
        case XK_7:
            return VeraKey::Num7;
        case XK_8:
            return VeraKey::Num8;
        case XK_9:
            return VeraKey::Num9;

        // Shifted Symbols
        case XK_exclam:
            return VeraKey::Exclamation;
        case XK_at:
            return VeraKey::At;
        case XK_numbersign:
            return VeraKey::Hash;
        case XK_dollar:
            return VeraKey::Dollar;
        case XK_percent:
            return VeraKey::Percent;
        case XK_asciicircum:
            return VeraKey::Caret;
        case XK_ampersand:
            return VeraKey::Ampersand;
        case XK_asterisk:
            return VeraKey::Asterisk;
        case XK_parenleft:
            return VeraKey::LeftParen;
        case XK_parenright:
            return VeraKey::RightParen;
        case XK_underscore:
            return VeraKey::Underscore;
        case XK_plus:
            return VeraKey::Plus;
        case XK_colon:
            return VeraKey::Colon;
        case XK_quotedbl:
            return VeraKey::Quote;
        case XK_less:
            return VeraKey::LessThan;
        case XK_greater:
            return VeraKey::GreaterThan;
        case XK_question:
            return VeraKey::Question;
        case XK_braceleft:
            return VeraKey::LeftBrace;
        case XK_braceright:
            return VeraKey::RightBrace;
        case XK_bar:
            return VeraKey::Pipe;
        case XK_asciitilde:
            return VeraKey::Tilde;

        // Base Symbols
        case XK_space:
            return VeraKey::Space;
        case XK_apostrophe:
            return VeraKey::Apostrophe;
        case XK_comma:
            return VeraKey::Comma;
        case XK_minus:
            return VeraKey::Minus;
        case XK_period:
            return VeraKey::Period;
        case XK_slash:
            return VeraKey::Slash;
        case XK_semicolon:
            return VeraKey::Semicolon;
        case XK_equal:
            return VeraKey::Equal;
        case XK_bracketleft:
            return VeraKey::LeftBracket;
        case XK_backslash:
            return VeraKey::Backslash;
        case XK_bracketright:
            return VeraKey::RightBracket;
        case XK_grave:
            return VeraKey::GraveAccent;

        // Control / Navigation
        case XK_Return:
            return VeraKey::Enter;
        case XK_Escape:
            return VeraKey::Escape;
        case XK_Tab:
            return VeraKey::Tab;
        case XK_BackSpace:
            return VeraKey::Backspace;
        case XK_Insert:
            return VeraKey::Insert;
        case XK_Delete:
            return VeraKey::Delete;
        case XK_Home:
            return VeraKey::Home;
        case XK_End:
            return VeraKey::End;
        case XK_Page_Up:
            return VeraKey::PageUp;
        case XK_Page_Down:
            return VeraKey::PageDown;
        case XK_Left:
            return VeraKey::Left;
        case XK_Right:
            return VeraKey::Right;
        case XK_Up:
            return VeraKey::Up;
        case XK_Down:
            return VeraKey::Down;

        // Modifiers
        case XK_Shift_L:
            return VeraKey::LeftShift;
        case XK_Shift_R:
            return VeraKey::RightShift;
        case XK_Control_L:
            return VeraKey::LeftCtrl;
        case XK_Control_R:
            return VeraKey::RightCtrl;
        case XK_Alt_L:
            return VeraKey::LeftAlt;
        case XK_Alt_R:
            return VeraKey::RightAlt;
        case XK_Super_L:
            return VeraKey::LeftSuper;
        case XK_Super_R:
            return VeraKey::RightSuper;

        // Locks / Systems
        case XK_Caps_Lock:
            return VeraKey::CapsLock;
        case XK_Scroll_Lock:
            return VeraKey::ScrollLock;
        case XK_Num_Lock:
            return VeraKey::NumLock;
        case XK_Print:
            return VeraKey::PrintScreen;
        case XK_Pause:
            return VeraKey::Pause;
        case XK_Menu:
            return VeraKey::Menu;

        // Function Keys
        case XK_F1:
            return VeraKey::F1;
        case XK_F2:
            return VeraKey::F2;
        case XK_F3:
            return VeraKey::F3;
        case XK_F4:
            return VeraKey::F4;
        case XK_F5:
            return VeraKey::F5;
        case XK_F6:
            return VeraKey::F6;
        case XK_F7:
            return VeraKey::F7;
        case XK_F8:
            return VeraKey::F8;
        case XK_F9:
            return VeraKey::F9;
        case XK_F10:
            return VeraKey::F10;
        case XK_F11:
            return VeraKey::F11;
        case XK_F12:
            return VeraKey::F12;

        // Keypad
        case XK_KP_0:
            return VeraKey::KP0;
        case XK_KP_1:
            return VeraKey::KP1;
        case XK_KP_2:
            return VeraKey::KP2;
        case XK_KP_3:
            return VeraKey::KP3;
        case XK_KP_4:
            return VeraKey::KP4;
        case XK_KP_5:
            return VeraKey::KP5;
        case XK_KP_6:
            return VeraKey::KP6;
        case XK_KP_7:
            return VeraKey::KP7;
        case XK_KP_8:
            return VeraKey::KP8;
        case XK_KP_9:
            return VeraKey::KP9;
        case XK_KP_Decimal:
            return VeraKey::KPDecimal;
        case XK_KP_Divide:
            return VeraKey::KPDivide;
        case XK_KP_Multiply:
            return VeraKey::KPMultiply;
        case XK_KP_Subtract:
            return VeraKey::KPSubtract;
        case XK_KP_Add:
            return VeraKey::KPAdd;
        case XK_KP_Enter:
            return VeraKey::KPEnter;
        case XK_KP_Equal:
            return VeraKey::KPEqual;

        // Media
        case XF86XK_AudioMute:
            return VeraKey::Mute;
        case XF86XK_AudioRaiseVolume:
            return VeraKey::VolumeUp;
        case XF86XK_AudioLowerVolume:
            return VeraKey::VolumeDown;

        default:
            return VeraKey::Unknown;
    }
}

VeraKey convertKeyEventToVeraKeyX11(X11Context& ctx, XKeyEvent& event) {
    KeySym ks = NoSymbol;

    // We use column index matching based on Shift / CapsLock status
    int col = 0;
    if (event.state & (ShiftMask | LockMask)) {
        col = 1;
    }

    // Try to get the active layout keysym with context modifiers
    ks = XkbKeycodeToKeysym(ctx.display, event.keycode, 0, col);

    // Fallback logic if column 1 yields null (like on Function keys or Control
    // maps)
    if (ks == NoSymbol) {
        ks = XkbKeycodeToKeysym(ctx.display, event.keycode, 0, 0);
    }

    return mapKeysym(ks);
}

uint32_t convertKeyEventToCodepointX11(X11Context& ctx, XKeyEvent& event) {
    std::array<char, 32> buffer{};
    KeySym keysym = NoSymbol;
    Status status;

    int len = 0;
    XIC targetXic = nullptr;

    auto it = ctx.windowsByXid.find(event.window);
    if (it != ctx.windowsByXid.end() && it->second) {
        targetXic = it->second->getXIC();
    }

    if (targetXic) {
        len = Xutf8LookupString(targetXic, &event, buffer.data(),
                                static_cast<int>(buffer.size()), &keysym,
                                &status);
    } else {
        len = XLookupString(&event, buffer.data(),
                            static_cast<int>(buffer.size()), &keysym, nullptr);
    }

    if (len <= 0) return 0;

    uint32_t codepoint = 0;
    auto u8Lead = static_cast<unsigned char>(buffer[0]);

    if (u8Lead < 0x80) {
        codepoint = u8Lead;
    } else if ((u8Lead & 0xE0) == 0xC0 && len >= 2) {
        codepoint = ((u8Lead & 0x1F) << 6) |
                    (static_cast<unsigned char>(buffer[1]) & 0x3F);
    } else if ((u8Lead & 0xF0) == 0xE0 && len >= 3) {
        codepoint = ((u8Lead & 0x0F) << 12) |
                    ((static_cast<unsigned char>(buffer[1]) & 0x3F) << 6) |
                    (static_cast<unsigned char>(buffer[2]) & 0x3F);
    } else if ((u8Lead & 0xF8) == 0xF0 && len >= 4) {
        codepoint = ((u8Lead & 0x07) << 18) |
                    ((static_cast<unsigned char>(buffer[1]) & 0x3F) << 12) |
                    ((static_cast<unsigned char>(buffer[2]) & 0x3F) << 6) |
                    (static_cast<unsigned char>(buffer[3]) & 0x3F);
    }

    return codepoint;
}
