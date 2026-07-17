#include "vera_win32.h"

#include <dwmapi.h>
#include <shellscalingapi.h>
#include <windowsx.h>

#include <string>
#include <vector>

#include "core/monitor/Monitor.h"

static std::wstring utf8ToWstring(const std::string& s) {
    if (s.empty()) return std::wstring{};
    int required = MultiByteToWideChar(CP_UTF8, 0, s.data(),
                                       static_cast<int>(s.size()), nullptr, 0);
    if (required == 0) return std::wstring{};
    std::wstring out;
    out.resize(required);
    MultiByteToWideChar(CP_UTF8, 0, s.data(), static_cast<int>(s.size()),
                        out.data(), required);
    return out;
}

static std::string wstringToUtf8(const std::wstring& s) {
    if (s.empty()) return std::string{};
    int required =
        WideCharToMultiByte(CP_UTF8, 0, s.data(), static_cast<int>(s.size()),
                            nullptr, 0, nullptr, nullptr);
    if (required == 0) return std::string{};
    std::string out;
    out.resize(required);
    WideCharToMultiByte(CP_UTF8, 0, s.data(), static_cast<int>(s.size()),
                        out.data(), required, nullptr, nullptr);
    return out;
}

static VeraKey translateWin32Key(WPARAM wparam, LPARAM lparam) {
    switch (wparam) {
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

        case VK_ESCAPE:
            return VeraKey::Escape;
        case VK_SPACE:
            return VeraKey::Space;
        case VK_RETURN:
            return VeraKey::Enter;
        case VK_TAB:
            return VeraKey::Tab;
        case VK_BACK:
            return VeraKey::Backspace;
        case VK_INSERT:
            return VeraKey::Insert;
        case VK_DELETE:
            return VeraKey::Delete;
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

        // Handle precise Left/Right modifier isolation via scan codes
        case VK_SHIFT: {
            UINT scancode = (lparam & 0x00ff0000) >> 16;
            UINT mappedVk = MapVirtualKeyW(scancode, MAPVK_VSC_TO_VK_EX);

            return (mappedVk == VK_RSHIFT) ? VeraKey::RightShift
                                           : VeraKey::LeftShift;
        }
        case VK_CONTROL:
            return (lparam & 0x01000000) ? VeraKey::RightCtrl
                                         : VeraKey::LeftCtrl;
        case VK_MENU:
            return (lparam & 0x01000000) ? VeraKey::RightAlt : VeraKey::LeftAlt;
        case VK_LWIN:
            return VeraKey::LeftSuper;
        case VK_RWIN:
            return VeraKey::RightSuper;

        default:
            return VeraKey::Unknown;
    }
}

VeraWindowHandle VeraWin32Window::generateUniqueHandle() {
    uint64_t id = s_next_handle_id.fetch_add(1, std::memory_order_relaxed);
    return VeraWindowHandle{id};
}

void VeraWin32Window::calculateWin32Styles(const VeraWindowInfo& info,
                                           DWORD& style, DWORD& exStyle) const {
    style = 0;
    exStyle = 0;

    if (info.fullscreenMode == FullScreenMode::Exclusive ||
        info.fullscreenMode == FullScreenMode::Borderless) {
        style |= WS_POPUP;
    } else if (info.customTitleBar) {
        style |= WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
        if (info.resizable) {
            style |= WS_THICKFRAME | WS_MAXIMIZEBOX;
        }
    } else {
        if (info.decorated) {
            style |= WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
            if (info.resizable) {
                style |= WS_THICKFRAME | WS_MAXIMIZEBOX;
            }
        } else {
            style |= WS_POPUP;
        }
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

void VeraWin32Window::calculateWindowDimensions(const VeraWindowInfo& info,
                                                DWORD style, DWORD exStyle,
                                                int& x, int& y, int& width,
                                                int& height) const {
    RECT rect = {0, 0, static_cast<LONG>(info.width),
                 static_cast<LONG>(info.height)};

    if (info.fullscreenMode == FullScreenMode::Windowed &&
        !info.customTitleBar && info.decorated) {
        AdjustWindowRectEx(&rect, style, FALSE, exStyle);
    }

    width = rect.right - rect.left;
    height = rect.bottom - rect.top;

    HMONITOR targetMonitor = nullptr;
    if (info.monitorIndex >= 0) {
        struct MonitorSearchContext {
            int target;
            int current;
            HMONITOR result;
        } context = {info.monitorIndex, 0, nullptr};

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

        targetMonitor = context.result;
    }

    if (!targetMonitor) {
        targetMonitor = MonitorFromPoint(POINT{0, 0}, MONITOR_DEFAULTTOPRIMARY);
    }

    MONITORINFO monitorInfo = {sizeof(MONITORINFO)};
    GetMonitorInfoW(targetMonitor, &monitorInfo);

    RECT targetArea = (info.fullscreenMode != FullScreenMode::Windowed)
                          ? monitorInfo.rcMonitor
                          : monitorInfo.rcWork;

    if (info.fullscreenMode != FullScreenMode::Windowed) {
        x = targetArea.left;
        y = targetArea.top;
        width = targetArea.right - targetArea.left;
        height = targetArea.bottom - targetArea.top;
        return;
    }

    if (info.centerOnMonitor) {
        int monitorWidth = targetArea.right - targetArea.left;
        int monitorHeight = targetArea.bottom - targetArea.top;
        x = targetArea.left + (monitorWidth - width) / 2;
        y = targetArea.top + (monitorHeight - height) / 2;
    } else {
        if (info.x.has_value()) {
            x = static_cast<int>(*info.x);
        } else {
            if ((style & WS_POPUP) != 0) {
                x = targetArea.left;
            } else {
                x = CW_USEDEFAULT;
            }
        }

        if (info.y.has_value()) {
            y = static_cast<int>(*info.y);
        } else {
            if ((style & WS_POPUP) != 0) {
                y = targetArea.top;
            } else {
                y = CW_USEDEFAULT;
            }
        }
    }
}

void VeraWin32Window::createNativeWindow(const VeraWindowInfo& info,
                                         DWORD style, DWORD ex_style, int x,
                                         int y, int width, int height) {
    HINSTANCE instance = GetModuleHandleW(nullptr);
    const wchar_t* className = L"VeraWindowClass";

    WNDCLASSEXW wndClass = {sizeof(WNDCLASSEXW)};
    if (!GetClassInfoExW(instance, className, &wndClass)) {
        wndClass.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
        wndClass.lpfnWndProc = VeraWin32Window::windowProcRouter;
        wndClass.hInstance = instance;
        wndClass.hCursor = LoadCursorW(nullptr, IDC_ARROW);
        wndClass.lpszClassName = className;

        RegisterClassExW(&wndClass);
    }

    std::wstring wideTitle(info.title.begin(), info.title.end());

    m_hwnd =
        CreateWindowExW(ex_style, className, wideTitle.c_str(), style, x, y,
                        width, height, nullptr, nullptr, instance, this);

    if (!m_hwnd) {
        return;
    }

    if (info.customTitleBar) {
        VeraHitTestRegions initialRegions{};
        initialRegions.dragRegion =
            VeraRect{0, 0, info.width, info.titleBarHeight};
        m_hit_test_regions = initialRegions;
        SetWindowPos(m_hwnd, nullptr, 0, 0, 0, 0,
                     SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE |
                         SWP_FRAMECHANGED);
        DWM_WINDOW_CORNER_PREFERENCE preference = DWMWCP_ROUND;
        DwmSetWindowAttribute(m_hwnd, DWMWA_WINDOW_CORNER_PREFERENCE,
                              &preference, sizeof(preference));
    }

    if (info.startVisible) {
        int showCommand = SW_SHOW;
        if (info.startMaximized &&
            info.fullscreenMode == FullScreenMode::Windowed) {
            showCommand = SW_SHOWMAXIMIZED;
        } else if (info.startMinimized &&
                   info.fullscreenMode == FullScreenMode::Windowed) {
            showCommand = SW_SHOWMINIMIZED;
        }

        ShowWindow(m_hwnd, showCommand);

        if (info.focusOnShow) {
            SetForegroundWindow(m_hwnd);
            SetFocus(m_hwnd);
        }
    }
}

LRESULT CALLBACK VeraWin32Window::windowProcRouter(HWND hwnd, UINT msg,
                                                   WPARAM wparam,
                                                   LPARAM lparam) {
    VeraWin32Window* window = nullptr;

    if (msg == WM_NCCREATE) {
        auto* createStruct = reinterpret_cast<CREATESTRUCTW*>(lparam);
        window =
            reinterpret_cast<VeraWin32Window*>(createStruct->lpCreateParams);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA,
                          reinterpret_cast<LONG_PTR>(window));
        window->m_hwnd = hwnd;
    } else {
        window = reinterpret_cast<VeraWin32Window*>(
            GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    }

    if (window) {
        return window->handleMessage(msg, wparam, lparam);
    }

    return DefWindowProcW(hwnd, msg, wparam, lparam);
}

VeraWin32Window::VeraWin32Window(const VeraWindowInfo& info) {
    m_handle = generateUniqueHandle();

    DWORD style = 0;
    DWORD exStyle = 0;
    calculateWin32Styles(info, style, exStyle);

    int x = CW_USEDEFAULT;
    int y = CW_USEDEFAULT;
    int width = static_cast<int>(info.width);
    int height = static_cast<int>(info.height);
    calculateWindowDimensions(info, style, exStyle, x, y, width, height);

    createNativeWindow(info, style, exStyle, x, y, width, height);
}

VeraWin32Window::~VeraWin32Window() {
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
    std::wstring wideTitle = utf8ToWstring(title);
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
    std::wstring widePath = utf8ToWstring(iconPath);

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

vera::core::monitor::VeraMonitorInfo VeraWin32Window::getCurrentMonitor()
    const {
    vera::core::monitor::VeraMonitorInfo info{};
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
        info.name = wstringToUtf8(wideName);

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
            auto* mmi = reinterpret_cast<MINMAXINFO*>(lparam);
            DWORD style =
                static_cast<DWORD>(GetWindowLongPtrW(m_hwnd, GWL_STYLE));
            DWORD exStyle =
                static_cast<DWORD>(GetWindowLongPtrW(m_hwnd, GWL_EXSTYLE));
            RECT rect = {0, 0, 0, 0};
            AdjustWindowRectEx(&rect, style, FALSE, exStyle);
            int borderW = rect.right - rect.left;
            int borderH = rect.bottom - rect.top;

            if (m_min_width) {
                mmi->ptMinTrackSize.x =
                    static_cast<LONG>(*m_min_width) + borderW;
            }
            if (m_min_height) {
                mmi->ptMinTrackSize.y =
                    static_cast<LONG>(*m_min_height) + borderH;
            }
            if (m_max_width) {
                mmi->ptMaxTrackSize.x =
                    static_cast<LONG>(*m_max_width) + borderW;
            }
            if (m_max_height) {
                mmi->ptMaxTrackSize.y =
                    static_cast<LONG>(*m_max_height) + borderH;
            }
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
                VeraKey key = translateWin32Key(wparam, lparam);
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
            if (!m_hit_test_regions.has_value()) {
                break;
            }

            int x = GET_X_LPARAM(lparam);
            int y = GET_Y_LPARAM(lparam);
            POINT pt = {x, y};
            ScreenToClient(m_hwnd, &pt);

            const int32_t mouseX = static_cast<int32_t>(pt.x);
            const int32_t mouseY = static_cast<int32_t>(pt.y);

            const int resizeBorderWidth = GetSystemMetrics(SM_CXSIZEFRAME) +
                                          GetSystemMetrics(SM_CXPADDEDBORDER);
            RECT rc;
            GetClientRect(m_hwnd, &rc);

            if (pt.y < resizeBorderWidth) {
                if (pt.x < resizeBorderWidth) return HTTOPLEFT;
                if (pt.x >= rc.right - resizeBorderWidth) return HTTOPRIGHT;
                return HTTOP;
            }
            if (pt.y >= rc.bottom - resizeBorderWidth) {
                if (pt.x < resizeBorderWidth) return HTBOTTOMLEFT;
                if (pt.x >= rc.right - resizeBorderWidth) {
                    return HTBOTTOMRIGHT;
                }
                return HTBOTTOM;
            }
            if (pt.x < resizeBorderWidth) return HTLEFT;
            if (pt.x >= rc.right - resizeBorderWidth) return HTRIGHT;

            const auto& regions = m_hit_test_regions.value();

            if (regions.closeButton.has_value()) {
                const auto& rect = regions.closeButton.value();
                if (mouseX >= static_cast<int32_t>(rect.x) &&
                    mouseX < static_cast<int32_t>(rect.x + rect.width) &&
                    mouseY >= static_cast<int32_t>(rect.y) &&
                    mouseY < static_cast<int32_t>(rect.y + rect.height)) {
                    return HTCLOSE;
                }
            }

            if (regions.maximizeButton.has_value()) {
                const auto& rect = regions.maximizeButton.value();
                if (mouseX >= static_cast<int32_t>(rect.x) &&
                    mouseX < static_cast<int32_t>(rect.x + rect.width) &&
                    mouseY >= static_cast<int32_t>(rect.y) &&
                    mouseY < static_cast<int32_t>(rect.y + rect.height)) {
                    return HTMAXBUTTON;
                }
            }

            if (regions.minimizeButton.has_value()) {
                const auto& rect = regions.minimizeButton.value();
                if (mouseX >= static_cast<int32_t>(rect.x) &&
                    mouseX < static_cast<int32_t>(rect.x + rect.width) &&
                    mouseY >= static_cast<int32_t>(rect.y) &&
                    mouseY < static_cast<int32_t>(rect.y + rect.height)) {
                    return HTMINBUTTON;
                }
            }

            if (regions.dragRegion.has_value()) {
                const auto& rect = regions.dragRegion.value();
                if (mouseX >= static_cast<int32_t>(rect.x) &&
                    mouseX < static_cast<int32_t>(rect.x + rect.width) &&
                    mouseY >= static_cast<int32_t>(rect.y) &&
                    mouseY < static_cast<int32_t>(rect.y + rect.height)) {
                    return HTCAPTION;
                }
            }

            return HTCLIENT;
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

void VeraWin32Window::setDestroyedNotifier(
    std::function<void(VeraWindowHandle)> notifier) {
    m_destroyedNotifier = std::move(notifier);
};
