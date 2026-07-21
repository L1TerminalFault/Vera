#include <bit>

#include "platform/win32/window/vera_win32.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>

LRESULT CALLBACK VeraWin32Window::windowProcRouter(HWND hwnd, UINT msg,
                                                   WPARAM wparam,
                                                   LPARAM lparam) {
    if (msg == WM_CREATE) {
        const auto* createStruct = std::bit_cast<CREATESTRUCTW*>(lparam);
        auto* window =
            std::bit_cast<VeraWin32Window*>(createStruct->lpCreateParams);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, std::bit_cast<LONG_PTR>(window));
        window->m_hwnd = hwnd;
        return window->handleMessage(msg, wparam, lparam);
    }
    if (auto* window = std::bit_cast<VeraWin32Window*>(
            GetWindowLongPtrW(hwnd, GWLP_USERDATA))) {
        return window->handleMessage(msg, wparam, lparam);
    }
    return DefWindowProcW(hwnd, msg, wparam, lparam);
}
