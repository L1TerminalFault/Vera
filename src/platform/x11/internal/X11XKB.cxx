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
        case XK_a:
        case XK_A:
            return VeraKey::A;
        case XK_b:
        case XK_B:
            return VeraKey::B;
        case XK_c:
        case XK_C:
            return VeraKey::C;
        case XK_d:
        case XK_D:
            return VeraKey::D;
        case XK_e:
        case XK_E:
            return VeraKey::E;
        case XK_f:
        case XK_F:
            return VeraKey::F;
        case XK_g:
        case XK_G:
            return VeraKey::G;
        case XK_h:
        case XK_H:
            return VeraKey::H;
        case XK_i:
        case XK_I:
            return VeraKey::I;
        case XK_j:
        case XK_J:
            return VeraKey::J;
        case XK_k:
        case XK_K:
            return VeraKey::K;
        case XK_l:
        case XK_L:
            return VeraKey::L;
        case XK_m:
        case XK_M:
            return VeraKey::M;
        case XK_n:
        case XK_N:
            return VeraKey::N;
        case XK_o:
        case XK_O:
            return VeraKey::O;
        case XK_p:
        case XK_P:
            return VeraKey::P;
        case XK_q:
        case XK_Q:
            return VeraKey::Q;
        case XK_r:
        case XK_R:
            return VeraKey::R;
        case XK_s:
        case XK_S:
            return VeraKey::S;
        case XK_t:
        case XK_T:
            return VeraKey::T;
        case XK_u:
        case XK_U:
            return VeraKey::U;
        case XK_v:
        case XK_V:
            return VeraKey::V;
        case XK_w:
        case XK_W:
            return VeraKey::W;
        case XK_x:
        case XK_X:
            return VeraKey::X;
        case XK_y:
        case XK_Y:
            return VeraKey::Y;
        case XK_z:
        case XK_Z:
            return VeraKey::Z;

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
        case XK_F13:
            return VeraKey::F13;
        case XK_F14:
            return VeraKey::F14;
        case XK_F15:
            return VeraKey::F15;
        case XK_F16:
            return VeraKey::F16;
        case XK_F17:
            return VeraKey::F17;
        case XK_F18:
            return VeraKey::F18;
        case XK_F19:
            return VeraKey::F19;
        case XK_F20:
            return VeraKey::F20;
        case XK_F21:
            return VeraKey::F21;
        case XK_F22:
            return VeraKey::F22;
        case XK_F23:
            return VeraKey::F23;
        case XK_F24:
            return VeraKey::F24;

        case XK_space:
            return VeraKey::Space;
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

VeraKey convertKeycodeToVeraKeyX11(X11Context& ctx, unsigned int keycode) {
    KeySym ks = XkbKeycodeToKeysym(ctx.display, keycode, 0, 0);
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
