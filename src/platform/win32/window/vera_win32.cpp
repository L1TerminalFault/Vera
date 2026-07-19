#include "vera_win32.h"

#include <shellscalingapi.h>
#include <windowsx.h>

#include <bit>
#include <string>
#include <vector>

#include "platform/win32/utils/keyTranslationMap.h"
#include "platform/win32/utils/win32_utils.h"
#include "platform/win32/utils/windowCreationUtils.h"

void VeraWin32Window::createNativeWindow(const VeraWindowInfo& info,
                                         DWORD style, DWORD ex_style, int x,
                                         int y, int width, int height) {
    HINSTANCE instance = GetModuleHandleW(nullptr);
    const wchar_t* className = L"VeraWindowClass";
    if (!utils::registerWindowClass(instance, className,
                                    VeraWin32Window::windowProcRouter)) {
        return;
    }
    std::wstring wideTitle(info.title.begin(), info.title.end());
    m_hwnd =
        CreateWindowExW(ex_style, className, wideTitle.c_str(), style, x, y,
                        width, height, nullptr, nullptr, instance, this);
    if (!m_hwnd) {
        return;
    }
    if (info.customTitleBar) {
        m_hit_test_regions = utils::initializeCustomTitleBar(
            m_hwnd, info.width, info.titleBarHeight);
    }
    utils::showAndFocusWindow(m_hwnd, info);
    // force it to refresh the window frame if custom title bar is enabled
    if (info.customTitleBar) {
        SetWindowPos(m_hwnd, nullptr, 0, 0, 0, 0,
                     SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE |
                         SWP_FRAMECHANGED);
    }
}

VeraWin32Window::VeraWin32Window(const VeraWindowInfo& info) {
    m_handle = utils::generateUniqueHandle();

    DWORD style = 0;
    DWORD exStyle = 0;
    utils::determineWindowStyles(info, style, exStyle);

    int x = CW_USEDEFAULT;
    int y = CW_USEDEFAULT;
    int width = static_cast<int>(info.width);
    int height = static_cast<int>(info.height);
    utils::resolveWindowDimensions(info, style, exStyle, x, y, width, height);

    createNativeWindow(info, style, exStyle, x, y, width, height);
}

VeraWin32Window::~VeraWin32Window() {
    if (m_destruction_callback) {
        m_destruction_callback(this);
    }
    if (m_hwnd) {
        DestroyWindow(m_hwnd);
        m_hwnd = nullptr;
    }
}

VeraNativeHandle VeraWin32Window::getNativeHandle() const {
    VeraNativeHandle handle{};
    handle.hwnd = m_hwnd;
    return handle;
}

void VeraWin32Window::setSize(uint32_t width, uint32_t height) {
    if (!m_hwnd) {
        return;
    }

    DWORD style = static_cast<DWORD>(GetWindowLongPtrW(m_hwnd, GWL_STYLE));
    DWORD exStyle = static_cast<DWORD>(GetWindowLongPtrW(m_hwnd, GWL_EXSTYLE));

    RECT rect = {0, 0, static_cast<LONG>(width), static_cast<LONG>(height)};
    AdjustWindowRectEx(&rect, style, FALSE, exStyle);

    int finalWidth = rect.right - rect.left;
    int finalHeight = rect.bottom - rect.top;

    SetWindowPos(m_hwnd, nullptr, 0, 0, finalWidth, finalHeight,
                 SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
}

void VeraWin32Window::setPosition(int32_t x, int32_t y) {
    if (!m_hwnd) {
        return;
    }

    SetWindowPos(m_hwnd, nullptr, x, y, 0, 0,
                 SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
}

void VeraWin32Window::setMinSize(uint32_t width, uint32_t height) {
    m_min_width = (width > 0) ? std::make_optional(width) : std::nullopt;
    m_min_height = (height > 0) ? std::make_optional(height) : std::nullopt;

    if (!m_hwnd) {
        return;
    }

    RECT rect;
    GetWindowRect(m_hwnd, &rect);
    int currentWidth = rect.right - rect.left;
    int currentHeight = rect.bottom - rect.top;

    bool needsResize = false;
    int targetWidth = currentWidth;
    int targetHeight = currentHeight;

    DWORD style = static_cast<DWORD>(GetWindowLongPtrW(m_hwnd, GWL_STYLE));
    DWORD exStyle = static_cast<DWORD>(GetWindowLongPtrW(m_hwnd, GWL_EXSTYLE));

    RECT clientAdjustment = {0, 0, 0, 0};
    AdjustWindowRectEx(&clientAdjustment, style, FALSE, exStyle);
    int borderWidth = (clientAdjustment.right - clientAdjustment.left);
    int borderHeight = (clientAdjustment.bottom - clientAdjustment.top);

    if (m_min_width &&
        (currentWidth - borderWidth) < static_cast<int>(*m_min_width)) {
        targetWidth = static_cast<int>(*m_min_width) + borderWidth;
        needsResize = true;
    }
    if (m_min_height &&
        (currentHeight - borderHeight) < static_cast<int>(*m_min_height)) {
        targetHeight = static_cast<int>(*m_min_height) + borderHeight;
        needsResize = true;
    }

    if (needsResize) {
        SetWindowPos(m_hwnd, nullptr, 0, 0, targetWidth, targetHeight,
                     SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
    }
}

void VeraWin32Window::setMaxSize(uint32_t width, uint32_t height) {
    m_max_width = (width > 0) ? std::make_optional(width) : std::nullopt;
    m_max_height = (height > 0) ? std::make_optional(height) : std::nullopt;

    if (!m_hwnd) {
        return;
    }

    RECT rect;
    GetWindowRect(m_hwnd, &rect);
    int currentWidth = rect.right - rect.left;
    int currentHeight = rect.bottom - rect.top;

    bool needsResize = false;
    int targetWidth = currentWidth;
    int targetHeight = currentHeight;

    DWORD style = static_cast<DWORD>(GetWindowLongPtrW(m_hwnd, GWL_STYLE));
    DWORD exStyle = static_cast<DWORD>(GetWindowLongPtrW(m_hwnd, GWL_EXSTYLE));

    RECT clientAdjustment = {0, 0, 0, 0};
    AdjustWindowRectEx(&clientAdjustment, style, FALSE, exStyle);
    int borderWidth = (clientAdjustment.right - clientAdjustment.left);
    int borderHeight = (clientAdjustment.bottom - clientAdjustment.top);

    if (m_max_width &&
        (currentWidth - borderWidth) > static_cast<int>(*m_max_width)) {
        targetWidth = static_cast<int>(*m_max_width) + borderWidth;
        needsResize = true;
    }
    if (m_max_height &&
        (currentHeight - borderHeight) > static_cast<int>(*m_max_height)) {
        targetHeight = static_cast<int>(*m_max_height) + borderHeight;
        needsResize = true;
    }

    if (needsResize) {
        SetWindowPos(m_hwnd, nullptr, 0, 0, targetWidth, targetHeight,
                     SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
    }
}

VeraWindowState VeraWin32Window::getState() const {
    VeraWindowState state{};
    if (!m_hwnd) {
        return state;
    }

    RECT rect;
    GetWindowRect(m_hwnd, &rect);
    state.x = rect.left;
    state.y = rect.top;
    state.width = static_cast<uint32_t>(rect.right - rect.left);
    state.height = static_cast<uint32_t>(rect.bottom - rect.top);

    WINDOWPLACEMENT wp = {sizeof(WINDOWPLACEMENT)};
    GetWindowPlacement(m_hwnd, &wp);
    state.isMinimized = (wp.showCmd == SW_SHOWMINIMIZED);
    state.isMaximized = (wp.showCmd == SW_SHOWMAXIMIZED);

    LONG_PTR style = GetWindowLongPtrW(m_hwnd, GWL_STYLE);
    state.isVisible = ((style & WS_VISIBLE) != 0);

    state.isFocused = (GetForegroundWindow() == m_hwnd);

    HMONITOR monitor = MonitorFromWindow(m_hwnd, MONITOR_DEFAULTTONEAREST);
    MONITORINFO mi = {sizeof(MONITORINFO)};
    GetMonitorInfoW(monitor, &mi);

    bool isPopup = ((style & WS_POPUP) != 0);
    bool matchesScreenBounds =
        (rect.left == mi.rcMonitor.left && rect.top == mi.rcMonitor.top &&
         rect.right == mi.rcMonitor.right &&
         rect.bottom == mi.rcMonitor.bottom);

    state.isFullscreen = (isPopup && matchesScreenBounds);

    return state;
}

void VeraWin32Window::show() {
    if (m_hwnd) {
        ShowWindow(m_hwnd, SW_SHOW);
    }
}

void VeraWin32Window::hide() {
    if (m_hwnd) {
        ShowWindow(m_hwnd, SW_HIDE);
    }
}

void VeraWin32Window::minimize() {
    if (m_hwnd) {
        ShowWindow(m_hwnd, SW_MINIMIZE);
    }
}

void VeraWin32Window::maximize() {
    if (m_hwnd) {
        ShowWindow(m_hwnd, SW_MAXIMIZE);
    }
}

void VeraWin32Window::restore() {
    if (m_hwnd) {
        ShowWindow(m_hwnd, SW_RESTORE);
    }
}

void VeraWin32Window::close() {
    if (m_hwnd) {
        PostMessageW(m_hwnd, WM_CLOSE, 0, 0);
    }
}

void VeraWin32Window::focus() {
    if (!m_hwnd) return;

    if (IsIconic(m_hwnd)) {
        ShowWindow(m_hwnd, SW_RESTORE);
    }

    SetForegroundWindow(m_hwnd);
    SetFocus(m_hwnd);
}

void VeraWin32Window::setTitle(const std::string& title) {
    if (!m_hwnd) return;
    std::wstring wideTitle = utils::utf8_to_wide(title);
    SetWindowTextW(m_hwnd, wideTitle.c_str());
}

void VeraWin32Window::setFullscreen(FullScreenMode mode) {
    if (!m_hwnd) return;

    DWORD style = static_cast<DWORD>(GetWindowLongPtrW(m_hwnd, GWL_STYLE));
    DWORD exStyle = static_cast<DWORD>(GetWindowLongPtrW(m_hwnd, GWL_EXSTYLE));

    if (mode != FullScreenMode::Windowed) {
        if (!m_is_fullscreen_cached) {
            GetWindowRect(m_hwnd, &m_saved_window_rect);
            m_saved_window_style = style;
            m_saved_window_ex_style = exStyle;
            m_is_fullscreen_cached = true;
        }

        style &= ~(WS_CAPTION | WS_THICKFRAME | WS_MINIMIZEBOX |
                   WS_MAXIMIZEBOX | WS_SYSMENU);
        style |= WS_POPUP;

        SetWindowLongPtrW(m_hwnd, GWL_STYLE, style);

        HMONITOR monitor = MonitorFromWindow(m_hwnd, MONITOR_DEFAULTTONEAREST);
        MONITORINFO mi = {sizeof(MONITORINFO)};
        GetMonitorInfoW(monitor, &mi);

        SetWindowPos(m_hwnd, HWND_TOP, mi.rcMonitor.left, mi.rcMonitor.top,
                     mi.rcMonitor.right - mi.rcMonitor.left,
                     mi.rcMonitor.bottom - mi.rcMonitor.top,
                     SWP_NOOWNERZORDER | SWP_FRAMECHANGED | SWP_SHOWWINDOW);
    } else {
        if (m_is_fullscreen_cached) {
            SetWindowLongPtrW(m_hwnd, GWL_STYLE, m_saved_window_style);
            SetWindowLongPtrW(m_hwnd, GWL_EXSTYLE, m_saved_window_ex_style);

            int x = m_saved_window_rect.left;
            int y = m_saved_window_rect.top;
            int w = m_saved_window_rect.right - m_saved_window_rect.left;
            int h = m_saved_window_rect.bottom - m_saved_window_rect.top;

            m_is_fullscreen_cached = false;

            SetWindowPos(m_hwnd, HWND_NOTOPMOST, x, y, w, h,
                         SWP_NOOWNERZORDER | SWP_FRAMECHANGED | SWP_SHOWWINDOW);
        }
    }
}

void VeraWin32Window::setAlwaysOnTop(bool value) {
    if (!m_hwnd) return;
    HWND orderTarget = value ? HWND_TOPMOST : HWND_NOTOPMOST;
    SetWindowPos(m_hwnd, orderTarget, 0, 0, 0, 0,
                 SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
}

void VeraWin32Window::setIcon(const std::string& iconPath) {
    if (!m_hwnd) return;
    std::wstring widePath = utils::utf8_to_wide(iconPath);

    HICON icon = reinterpret_cast<HICON>(
        LoadImageW(nullptr, widePath.c_str(), IMAGE_ICON, 0, 0,
                   LR_LOADFROMFILE | LR_DEFAULTSIZE | LR_SHARED));

    if (icon) {
        SendMessageW(m_hwnd, WM_SETICON, ICON_SMALL,
                     reinterpret_cast<LPARAM>(icon));
        SendMessageW(m_hwnd, WM_SETICON, ICON_BIG,
                     reinterpret_cast<LPARAM>(icon));
    }
}

void VeraWin32Window::setTitlebarHitTestRegions(
    const VeraHitTestRegions& regions) {
    m_hit_test_regions = regions;
}

void VeraWin32Window::setResizeCallback(
    std::function<void(uint32_t, uint32_t)> callback) {
    m_resize_callback = callback;
}

void VeraWin32Window::setMoveCallback(
    std::function<void(int32_t, int32_t)> callback) {
    m_move_callback = callback;
}

void VeraWin32Window::setCloseRequestCallback(std::function<bool()> callback) {
    m_close_request_callback = callback;
}

void VeraWin32Window::setFocusChangeCallback(
    std::function<void(bool)> callback) {
    m_focus_change_callback = callback;
}

void VeraWin32Window::setDpiChangeCallback(
    std::function<void(float)> callback) {
    m_dpi_change_callback = callback;
}

void VeraWin32Window::setKeyCallback(
    std::function<void(VeraKey, bool, bool)> callback) {
    m_key_callback = callback;
}

void VeraWin32Window::setMouseButtonCallback(
    std::function<void(VeraMouseButton, bool)> callback) {
    m_mouse_button_callback = callback;
}

void VeraWin32Window::setMouseMoveCallback(
    std::function<void(double, double)> callback) {
    m_mouse_move_callback = callback;
}

void VeraWin32Window::setScrollCallback(
    std::function<void(double, double)> callback) {
    m_scroll_callback = callback;
}

void VeraWin32Window::setCharCallback(std::function<void(uint32_t)> callback) {
    m_char_callback = callback;
}

void VeraWin32Window::setCursorMode(VeraCursorMode mode) {
    m_cursor_mode = mode;
    if (!m_hwnd) return;

    if (mode == VeraCursorMode::Disabled) {
        SetCapture(m_hwnd);

        RECT rect;
        GetClientRect(m_hwnd, &rect);
        MapWindowPoints(m_hwnd, nullptr, reinterpret_cast<POINT*>(&rect), 2);
        ClipCursor(&rect);
    } else {
        ClipCursor(nullptr);
        ReleaseCapture();
    }

    POINT pt;
    GetCursorPos(&pt);
    SetCursorPos(pt.x, pt.y);
}

void VeraWin32Window::setCursorShape(VeraCursorShape shape) {
    m_cursor_shape = shape;

    LPCWSTR win32CursorId = IDC_ARROW;
    switch (shape) {
        case VeraCursorShape::IBeam:
            win32CursorId = IDC_IBEAM;
            break;
        case VeraCursorShape::Crosshair:
            win32CursorId = IDC_CROSS;
            break;
        case VeraCursorShape::Hand:
            win32CursorId = IDC_HAND;
            break;
        case VeraCursorShape::HResize:
            win32CursorId = IDC_SIZEWE;
            break;
        case VeraCursorShape::VResize:
            win32CursorId = IDC_SIZENS;
            break;
        case VeraCursorShape::CornerResizeNWSE:
            win32CursorId = IDC_SIZENWSE;
            break;
        case VeraCursorShape::CornerResizeNESW:
            win32CursorId = IDC_SIZENESW;
            break;
        case VeraCursorShape::NotAllowed:
            win32CursorId = IDC_NO;
            break;
        default:
            break;
    }

    m_current_hcursor = LoadCursorW(nullptr, win32CursorId);

    if (m_hwnd && GetForegroundWindow() == m_hwnd) {
        SetCursor(m_current_hcursor ? m_current_hcursor
                                    : LoadCursorW(nullptr, IDC_ARROW));
    }
}

VeraMonitorInfo VeraWin32Window::getCurrentMonitor() const {
    VeraMonitorInfo info{};
    if (!m_hwnd) {
        return info;
    }

    HMONITOR monitor = MonitorFromWindow(m_hwnd, MONITOR_DEFAULTTONEAREST);
    if (!monitor) {
        return info;
    }

    MONITORINFOEXW mi = {};
    mi.cbSize = sizeof(MONITORINFOEXW);
    if (GetMonitorInfoW(monitor, &mi)) {
        std::wstring wideName(mi.szDevice);
        info.name = utils::wide_to_utf8(wideName);

        info.x = mi.rcMonitor.left;
        info.y = mi.rcMonitor.top;
        info.workAreaX = mi.rcWork.left;
        info.workAreaY = mi.rcWork.top;
        info.workAreaWidth =
            static_cast<uint32_t>(mi.rcWork.right - mi.rcWork.left);
        info.workAreaHeight =
            static_cast<uint32_t>(mi.rcWork.bottom - mi.rcWork.top);
        info.isPrimary = ((mi.dwFlags & MONITORINFOF_PRIMARY) != 0);
    }

    UINT dpiX = 96;
    UINT dpiY = 96;
    if (SUCCEEDED(GetDpiForMonitor(monitor, MDT_EFFECTIVE_DPI, &dpiX, &dpiY))) {
        info.dpiScale = static_cast<float>(dpiX) / 96.0f;
    } else {
        info.dpiScale = 1.0f;
    }

    DEVMODEW dm = {};
    dm.dmSize = sizeof(DEVMODEW);
    if (EnumDisplaySettingsW(mi.szDevice, ENUM_CURRENT_SETTINGS, &dm)) {
        info.refreshRateHz = dm.dmDisplayFrequency;
    }

    return info;
}
