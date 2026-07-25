#include <bit>

#include "platform/win32/utils/eventUtils.h"
#include "platform/win32/utils/keyTranslationMap.h"
#include "platform/win32/window/vera_win32.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>
#include <windowsx.h>

LRESULT VeraWin32Window::handleMessage(UINT msg, WPARAM wparam, LPARAM lparam) {
    switch (msg) {
        case WM_NCCALCSIZE: {
            if (wparam == TRUE && m_hit_test_regions.has_value() &&
                m_hit_test_regions->dragRegion.has_value()) {
                return 0;
            }
            break;
        }
        case WM_GETMINMAXINFO: {
            utils::handleMinMaxInfo(m_hwnd, lparam, m_min_width, m_min_height,
                                    m_max_width, m_max_height);
            return 0;
        }

        case WM_SIZE: {
            if (m_resize_callback && wparam != SIZE_MINIMIZED) {
                m_resize_callback(LOWORD(lparam), HIWORD(lparam));
            }
            return 0;
        }

        case WM_MOVE: {
            if (m_move_callback) {
                m_move_callback(static_cast<int16_t>(LOWORD(lparam)),
                                static_cast<int16_t>(HIWORD(lparam)));
            }
            return 0;
        }

        case WM_ACTIVATE: {
            if (m_focus_change_callback) {
                m_focus_change_callback(LOWORD(wparam) != WA_INACTIVE);
            }
            return 0;
        }

        case WM_CLOSE: {
            if (m_close_request_callback) {
                if (!m_close_request_callback()) {
                    return 0;
                }
            }
            DestroyWindow(m_hwnd);
            return 0;
        }

        case WM_DESTROY: {
            m_hwnd = nullptr;
            notifyDestroyed();
            return 0;
        }

        case WM_KEYDOWN:
        case WM_SYSKEYDOWN:
        case WM_KEYUP:
        case WM_SYSKEYUP: {
            if (m_key_callback) {
                bool pressed = (msg == WM_KEYDOWN || msg == WM_SYSKEYDOWN);
                bool repeat = pressed && ((lparam & 0x40000000) != 0);
                VeraKey key = utils::translateWin32KeyToVeraKey(wparam, lparam);
                if (key != VeraKey::Unknown) {
                    m_key_callback(key, pressed, repeat);
                }
            }
            return 0;
        }

        case WM_CHAR: {
            if (m_char_callback) {
                m_char_callback(static_cast<uint32_t>(wparam));
            }
            return 0;
        }

        case WM_MOUSEMOVE: {
            int x = LOWORD(lparam);
            int y = HIWORD(lparam);

            if (!m_mouse_tracked) {
                TRACKMOUSEEVENT tme = {sizeof(TRACKMOUSEEVENT), TME_LEAVE,
                                       m_hwnd, HOVER_DEFAULT};
                TrackMouseEvent(&tme);
                m_mouse_tracked = true;
            }

            if (m_mouse_move_callback) {
                m_mouse_move_callback(static_cast<double>(x),
                                      static_cast<double>(y));
            }
            return 0;
        }
        case WM_MOUSELEAVE: {
            m_mouse_tracked = false;
            return 0;
        }

        case WM_LBUTTONDOWN:
        case WM_LBUTTONUP:
        case WM_RBUTTONDOWN:
        case WM_RBUTTONUP:
        case WM_MBUTTONDOWN:
        case WM_MBUTTONUP: {
            if (m_mouse_button_callback) {
                VeraMouseButton btn = VeraMouseButton::Left;
                if (msg == WM_RBUTTONDOWN || msg == WM_RBUTTONUP) {
                    btn = VeraMouseButton::Right;
                }
                if (msg == WM_MBUTTONDOWN || msg == WM_MBUTTONUP) {
                    btn = VeraMouseButton::Middle;
                }

                bool pressed = (msg == WM_LBUTTONDOWN ||
                                msg == WM_RBUTTONDOWN || msg == WM_MBUTTONDOWN);
                m_mouse_button_callback(btn, pressed);
            }
            return 0;
        }

        case WM_MOUSEWHEEL: {
            if (m_scroll_callback) {
                double offset =
                    static_cast<double>(GET_WHEEL_DELTA_WPARAM(wparam)) /
                    WHEEL_DELTA;
                m_scroll_callback(0.0, offset);
            }
            return 0;
        }

        case WM_PAINT: {
            PAINTSTRUCT ps;
            BeginPaint(m_hwnd, &ps);
            EndPaint(m_hwnd, &ps);
            return 0;
        }
        case WM_ERASEBKGND: {
            return 1;
        }

        case WM_SETCURSOR: {
            int hit = LOWORD(lparam);
            if (hit == HTCLIENT) {
                HCURSOR cur = m_current_hcursor
                                  ? m_current_hcursor
                                  : LoadCursorW(nullptr, IDC_ARROW);
                SetCursor(cur);
                return TRUE;
            }
            break;
        }
        case WM_NCHITTEST: {
            return utils::executeNonClientHitTest(m_hit_test_regions, m_hwnd,
                                                  GET_X_LPARAM(lparam),
                                                  GET_Y_LPARAM(lparam));
        }
        case WM_DPICHANGED: {
            int dpiX = LOWORD(wparam);
            if (m_dpi_change_callback) {
                m_dpi_change_callback(static_cast<float>(dpiX) / 96.0f);
            }
            if (lparam) {
                RECT* suggested = reinterpret_cast<RECT*>(lparam);
                SetWindowPos(m_hwnd, nullptr, suggested->left, suggested->top,
                             suggested->right - suggested->left,
                             suggested->bottom - suggested->top,
                             SWP_NOZORDER | SWP_NOACTIVATE);
            }
            return 0;
        }
    }

    return DefWindowProcW(m_hwnd, msg, wparam, lparam);
}
