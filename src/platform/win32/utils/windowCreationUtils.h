#pragma once
#include "core/window/WindowTypes.h"
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <dwmapi.h>

#include <string>

namespace utils {

/**
 * @brief Generates a stable, unique window handle sequentially.
 * Only visible inside this file.
 */
VeraWindowHandle generateUniqueHandle();

/**
 * @brief Calculates the outer window dimensions needed to satisfy the requested
 * client canvas size.
 */
inline void calculateAdjustedSize(const VeraWindowInfo& info, DWORD style,
                                  DWORD exStyle, int& outWidth,
                                  int& outHeight) {
    RECT rect = {0, 0, static_cast<LONG>(info.width),
                 static_cast<LONG>(info.height)};

    if (info.fullscreenMode == FullScreenMode::Windowed &&
        !info.customTitleBar && info.decorated) {
        AdjustWindowRectEx(&rect, style, FALSE, exStyle);
    }

    outWidth = rect.right - rect.left;
    outHeight = rect.bottom - rect.top;
}
/**
 * @brief Searches for and returns the target physical monitor handle by index.
 */
inline HMONITOR findTargetMonitor(int monitorIndex) {
    if (monitorIndex < 0) {
        return MonitorFromPoint(POINT{0, 0}, MONITOR_DEFAULTTOPRIMARY);
    }

    struct MonitorSearchContext {
        int target;
        int current;
        HMONITOR result;
    } context = {monitorIndex, 0, nullptr};

    EnumDisplayMonitors(
        nullptr, nullptr,
        [](HMONITOR monitor, HDC, LPRECT, LPARAM param) -> BOOL {
            auto* ctx = reinterpret_cast<MonitorSearchContext*>(param);
            if (ctx->current == ctx->target) {
                ctx->result = monitor;
                return FALSE;
            }
            ctx->current++;
            return TRUE;
        },
        reinterpret_cast<LPARAM>(&context));

    return context.result
               ? context.result
               : MonitorFromPoint(POINT{0, 0}, MONITOR_DEFAULTTOPRIMARY);
}

/**
 * @brief Calculates the top-left (x, y) coordinates of the window on the
 * target monitor.
 */
inline void calculateWindowPosition(const VeraWindowInfo& info, DWORD style,
                                    const RECT& targetArea, int width,
                                    int height, int& outX, int& outY) {
    if (info.centerOnMonitor) {
        int monitorWidth = targetArea.right - targetArea.left;
        int monitorHeight = targetArea.bottom - targetArea.top;
        outX = targetArea.left + (monitorWidth - width) / 2;
        outY = targetArea.top + (monitorHeight - height) / 2;
    } else {
        if (info.x.has_value()) {
            outX = static_cast<int>(*info.x);
        } else {
            outX = ((style & WS_POPUP) != 0) ? targetArea.left : CW_USEDEFAULT;
        }

        if (info.y.has_value()) {
            outY = static_cast<int>(*info.y);
        } else {
            outY = ((style & WS_POPUP) != 0) ? targetArea.top : CW_USEDEFAULT;
        }
    }
}
/**
 * @brief Determines the appropriate window styles (style and extended style)
 * based on the provided VeraWindowInfo.
 */
inline void determineWindowStyles(const VeraWindowInfo& info, DWORD& style,
                                  DWORD& exStyle) {
    style = 0;
    exStyle = 0;

    if (info.customTitleBar || info.decorated) {
        style |= WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
        if (info.resizable) {
            style |= WS_THICKFRAME | WS_MAXIMIZEBOX;
        }
    } else {
        style |= WS_POPUP;
    }

    if (info.alwaysOnTop) {
        exStyle |= WS_EX_TOPMOST;
    }

    if (info.transparentFramebuffer) {
        exStyle |= WS_EX_LAYERED;
    }

    if (info.startMaximized &&
        info.fullscreenMode == FullScreenMode::Windowed) {
        style |= WS_MAXIMIZE;
    }

    if (info.startMinimized &&
        info.fullscreenMode == FullScreenMode::Windowed) {
        style |= WS_MINIMIZE;
    }
}
/**
 * @brief Resolves the final window dimensions and position based on the
 * provided VeraWindowInfo, style, and extended style.
 */
inline void resolveWindowDimensions(const VeraWindowInfo& info, DWORD style,
                                    DWORD exStyle, int& x, int& y, int& width,
                                    int& height) {
    utils::calculateAdjustedSize(info, style, exStyle, width, height);

    HMONITOR targetMonitor = utils::findTargetMonitor(info.monitorIndex);
    MONITORINFO monitorInfo = {sizeof(MONITORINFO)};
    GetMonitorInfoW(targetMonitor, &monitorInfo);

    const bool isWindowed = (info.fullscreenMode == FullScreenMode::Windowed);
    const RECT targetArea =
        isWindowed ? monitorInfo.rcWork : monitorInfo.rcMonitor;

    if (!isWindowed) {
        x = targetArea.left;
        y = targetArea.top;
        width = targetArea.right - targetArea.left;
        height = targetArea.bottom - targetArea.top;
    } else {
        utils::calculateWindowPosition(info, style, targetArea, width, height,
                                       x, y);
    }
}

/**
 * @brief Registers the Win32 window class if it hasn't been registered yet.
 * @return True if registration succeeded or already exists, false otherwise.
 */
[[nodiscard]] inline bool registerWindowClass(HINSTANCE instance, const wchar_t* className,
                                WNDPROC wndProc) {
    WNDCLASSEXW wndClass{};
    // if already registered check
    if (GetClassInfoExW(instance, className, &wndClass)) {
        return true;
    }

    wndClass = {.cbSize = sizeof(WNDCLASSEXW),
                .style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC,
                .lpfnWndProc = wndProc,
                .hInstance = instance,
                .hCursor = LoadCursorW(nullptr, IDC_ARROW),
                .lpszClassName = className};

    return RegisterClassExW(&wndClass) != 0;
}

/**
 * @brief Configures custom title bar regions and DWM window corner preferences.
 * @return The initial VeraHitTestRegions configured for the window.
 */
[[nodiscard]] inline VeraHitTestRegions initializeCustomTitleBar(HWND hwnd, uint32_t width,
                                                   uint32_t titleBarHeight) {
    // forces it to recalculate
    SetWindowPos(hwnd, nullptr, 0, 0, 0, 0,
                 SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE |
                     SWP_FRAMECHANGED);
    // opt in to corner rounding
    // TODO: should be customizable by user of vera set to a flag
    DWM_WINDOW_CORNER_PREFERENCE preference = DWMWCP_ROUND;
    DwmSetWindowAttribute(hwnd, DWMWA_WINDOW_CORNER_PREFERENCE, &preference,
                          sizeof(preference));

    return VeraHitTestRegions{
        .dragRegion =
            VeraRect{.x = 0,
                     .y = 0,
                     .width = static_cast<uint32_t>(width),
                     .height = static_cast<uint32_t>(titleBarHeight)}};
}

/**
 * @brief Handles showing, maximizing, minimizing, and focusing the window based
 * on initial info flags.
 */
inline void showAndFocusWindow(HWND hwnd, const VeraWindowInfo& info) {
    if (!info.startVisible) {
        return;
    }
    int showCommand = SW_SHOW;

    if (info.fullscreenMode == FullScreenMode::Windowed) {
        if (info.startMaximized) {
            showCommand = SW_SHOWMAXIMIZED;
        } else if (info.startMinimized) {
            showCommand = SW_SHOWMINIMIZED;
        }
    }
    ShowWindow(hwnd, showCommand);
    if (info.focusOnShow) {
        SetForegroundWindow(hwnd);
        SetFocus(hwnd);
    }
}
}  // namespace utils