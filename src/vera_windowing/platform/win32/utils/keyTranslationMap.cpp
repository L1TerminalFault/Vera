#include "keyTranslationMap.h"

namespace utils {
VeraKey translateWin32KeyToVeraKey(WPARAM wparam, LPARAM lparam) {
    switch (wparam) {
        // Alphabet Keys
        case 'A':
            return VeraKey::A;
        case 'B':
            return VeraKey::B;
        case 'C':
            return VeraKey::C;
        case 'D':
            return VeraKey::D;
        case 'E':
            return VeraKey::E;
        case 'F':
            return VeraKey::F;
        case 'G':
            return VeraKey::G;
        case 'H':
            return VeraKey::H;
        case 'I':
            return VeraKey::I;
        case 'J':
            return VeraKey::J;
        case 'K':
            return VeraKey::K;
        case 'L':
            return VeraKey::L;
        case 'M':
            return VeraKey::M;
        case 'N':
            return VeraKey::N;
        case 'O':
            return VeraKey::O;
        case 'P':
            return VeraKey::P;
        case 'Q':
            return VeraKey::Q;
        case 'R':
            return VeraKey::R;
        case 'S':
            return VeraKey::S;
        case 'T':
            return VeraKey::T;
        case 'U':
            return VeraKey::U;
        case 'V':
            return VeraKey::V;
        case 'W':
            return VeraKey::W;
        case 'X':
            return VeraKey::X;
        case 'Y':
            return VeraKey::Y;
        case 'Z':
            return VeraKey::Z;

        // Top Row Number Keys
        case '0':
            return VeraKey::Num0;
        case '1':
            return VeraKey::Num1;
        case '2':
            return VeraKey::Num2;
        case '3':
            return VeraKey::Num3;
        case '4':
            return VeraKey::Num4;
        case '5':
            return VeraKey::Num5;
        case '6':
            return VeraKey::Num6;
        case '7':
            return VeraKey::Num7;
        case '8':
            return VeraKey::Num8;
        case '9':
            return VeraKey::Num9;

        // Functional System Keys
        case VK_ESCAPE:
            return VeraKey::Escape;
        case VK_SPACE:
            return VeraKey::Space;
        case VK_TAB:
            return VeraKey::Tab;
        case VK_BACK:
            return VeraKey::Backspace;
        case VK_INSERT:
            return VeraKey::Insert;
        case VK_DELETE:
            return VeraKey::Delete;
        case VK_HOME:
            return VeraKey::Home;
        case VK_END:
            return VeraKey::End;
        case VK_PRIOR:
            return VeraKey::PageUp;
        case VK_NEXT:
            return VeraKey::PageDown;
        case VK_LEFT:
            return VeraKey::Left;
        case VK_RIGHT:
            return VeraKey::Right;
        case VK_UP:
            return VeraKey::Up;
        case VK_DOWN:
            return VeraKey::Down;
        case VK_CAPITAL:
            return VeraKey::CapsLock;
        case VK_NUMLOCK:
            return VeraKey::NumLock;
        case VK_SCROLL:
            return VeraKey::ScrollLock;
        case VK_SNAPSHOT:
            return VeraKey::PrintScreen;
        case VK_PAUSE:
            return VeraKey::Pause;
        case VK_APPS:
            return VeraKey::Menu;

        // return key with a special check to check if its the numpad enter or
        // the normal enter key
        case VK_RETURN:
            return (lparam & 0x01000000) ? VeraKey::KPEnter : VeraKey::Enter;

        // Fn keys
        case VK_F1:
            return VeraKey::F1;
        case VK_F2:
            return VeraKey::F2;
        case VK_F3:
            return VeraKey::F3;
        case VK_F4:
            return VeraKey::F4;
        case VK_F5:
            return VeraKey::F5;
        case VK_F6:
            return VeraKey::F6;
        case VK_F7:
            return VeraKey::F7;
        case VK_F8:
            return VeraKey::F8;
        case VK_F9:
            return VeraKey::F9;
        case VK_F10:
            return VeraKey::F10;
        case VK_F11:
            return VeraKey::F11;
        case VK_F12:
            return VeraKey::F12;
        case VK_F13:
            return VeraKey::F13;
        case VK_F14:
            return VeraKey::F14;
        case VK_F15:
            return VeraKey::F15;
        case VK_F16:
            return VeraKey::F16;
        case VK_F17:
            return VeraKey::F17;
        case VK_F18:
            return VeraKey::F18;
        case VK_F19:
            return VeraKey::F19;
        case VK_F20:
            return VeraKey::F20;
        case VK_F21:
            return VeraKey::F21;
        case VK_F22:
            return VeraKey::F22;
        case VK_F23:
            return VeraKey::F23;
        case VK_F24:
            return VeraKey::F24;

        // modifier keys with a special check to check if its the left or
        // right version of the key
        case VK_SHIFT: {
            UINT scancode = (lparam & 0x00ff0000) >> 16;
            UINT mappedVk = MapVirtualKeyW(scancode, MAPVK_VSC_TO_VK_EX);
            return (mappedVk == VK_RSHIFT) ? VeraKey::RightShift
                                           : VeraKey::LeftShift;
        }
        case VK_CONTROL:
            return (lparam & 0x01000000) ? VeraKey::RightCtrl
                                         : VeraKey::LeftCtrl;
        case VK_MENU:  // ALT key mapping
            return (lparam & 0x01000000) ? VeraKey::RightAlt : VeraKey::LeftAlt;
        case VK_LWIN:
            return VeraKey::LeftSuper;
        case VK_RWIN:
            return VeraKey::RightSuper;

        // punctuation and other keys
        case VK_OEM_7:
            return VeraKey::Apostrophe;  // ' " on US keyboard
        case VK_OEM_COMMA:
            return VeraKey::Comma;  // , <
        case VK_OEM_MINUS:
            return VeraKey::Minus;  // - _
        case VK_OEM_PERIOD:
            return VeraKey::Period;  // . >
        case VK_OEM_2:
            return VeraKey::Slash;  // / ? on US keyboard
        case VK_OEM_1:
            return VeraKey::Semicolon;  // ; : on US keyboard
        case VK_OEM_PLUS:
            return VeraKey::Equal;  // = +
        case VK_OEM_4:
            return VeraKey::LeftBracket;  // [ { on US keyboard
        case VK_OEM_5:
            return VeraKey::Backslash;  // \ | on US keyboard
        case VK_OEM_6:
            return VeraKey::RightBracket;  // ] } on US keyboard
        case VK_OEM_3:
            return VeraKey::GraveAccent;  // ` ~ on US keyboard

        // Numeric Numpad keys
        case VK_NUMPAD0:
            return VeraKey::KP0;
        case VK_NUMPAD1:
            return VeraKey::KP1;
        case VK_NUMPAD2:
            return VeraKey::KP2;
        case VK_NUMPAD3:
            return VeraKey::KP3;
        case VK_NUMPAD4:
            return VeraKey::KP4;
        case VK_NUMPAD5:
            return VeraKey::KP5;
        case VK_NUMPAD6:
            return VeraKey::KP6;
        case VK_NUMPAD7:
            return VeraKey::KP7;
        case VK_NUMPAD8:
            return VeraKey::KP8;
        case VK_NUMPAD9:
            return VeraKey::KP9;
        case VK_DECIMAL:
            return VeraKey::KPDecimal;
        case VK_DIVIDE:
            return VeraKey::KPDivide;
        case VK_MULTIPLY:
            return VeraKey::KPMultiply;
        case VK_SUBTRACT:
            return VeraKey::KPSubtract;
        case VK_ADD:
            return VeraKey::KPAdd;
        case VK_OEM_NEC_EQUAL:
            return VeraKey::KPEqual;

        // media keys
        case VK_VOLUME_UP:
            return VeraKey::VolumeUp;
        case VK_VOLUME_DOWN:
            return VeraKey::VolumeDown;
        case VK_VOLUME_MUTE:
            return VeraKey::Mute;

        default:
            return VeraKey::Unknown;
    }
}
}  // namespace utils