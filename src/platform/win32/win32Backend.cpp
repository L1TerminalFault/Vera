#include "platform/win32/Win32Backend.h"

#include <shellscalingapi.h>

#include <cstring>

#include "platform/win32/intern.h"
#include "platform/win32/utils/win32_utils.h"
#include "platform/win32/window/vera_win32.h"

std::expected<std::unique_ptr<VeraWindow>, VeraError>
Win32Backend::createWindow(const VeraWindowInfo& info) {
    std::unique_ptr<VeraWindow> window =
        std::make_unique<VeraWin32Window>(info);
    if (!window->getNativeHandle().hwnd) {
        return std::unexpected(VeraError{VeraErrorType::WindowCreationFailed,
                                         "CreateWindowExW failed"});
    }
    return window;
}

void Win32Backend::pollEvents() {
    MSG msg;
    while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    };
}
void Win32Backend::waitEvents() { vera::internal::waitPlatformEvents(); }
void Win32Backend::waitEventsTimeout(double timeoutSeconds) {
    vera::internal::waitPlatformEventsTimeout(timeoutSeconds);
}

void Win32Backend::setQuitRequestCallback(std::function<bool()> callback) {
    m_quitRequestCallback = std::move(callback);
}
void Win32Backend::setDisplayChangeCallback(std::function<void()> callback) {
    m_displayChangeCallback = std::move(callback);
}
void Win32Backend::setSystemThemeChangeCallback(
    std::function<void(VeraSystemTheme)> callback) {
    m_themeChangeCallback = std::move(callback);
}

static VeraMonitorInfo toVeraMonitor(HMONITOR hMonitor, bool isPrimary) {
    MONITORINFOEXW info{};
    info.cbSize = sizeof(info);
    GetMonitorInfoW(hMonitor, &info);

    UINT dpiX = 96, dpiY = 96;
    GetDpiForMonitor(hMonitor, MDT_EFFECTIVE_DPI, &dpiX, &dpiY);

    VeraMonitorInfo monitor;
    monitor.name = vera::platform::win32::wide_to_utf8(info.szDevice);
    monitor.x = info.rcMonitor.left;
    monitor.y = info.rcMonitor.top;
    monitor.workAreaX = info.rcWork.left;
    monitor.workAreaY = info.rcWork.top;
    monitor.workAreaWidth =
        static_cast<uint32_t>(info.rcWork.right - info.rcWork.left);
    monitor.workAreaHeight =
        static_cast<uint32_t>(info.rcWork.bottom - info.rcWork.top);
    monitor.dpiScale = static_cast<float>(dpiX) / 96.0f;
    monitor.isPrimary = isPrimary;

    DEVMODEW devMode{};
    devMode.dmSize = sizeof(devMode);
    monitor.refreshRateHz =
        EnumDisplaySettingsW(info.szDevice, ENUM_CURRENT_SETTINGS, &devMode)
            ? devMode.dmDisplayFrequency
            : 60;

    return monitor;
}

static BOOL CALLBACK enumMonitorProc(HMONITOR hMonitor, HDC, LPRECT,
                                     LPARAM lparam) {
    auto* out = reinterpret_cast<std::vector<VeraMonitorInfo>*>(lparam);
    MONITORINFOEXW info{};
    info.cbSize = sizeof(info);
    GetMonitorInfoW(hMonitor, &info);
    bool isPrimary = (info.dwFlags & MONITORINFOF_PRIMARY) != 0;
    out->push_back(toVeraMonitor(hMonitor, isPrimary));
    return TRUE;
}

std::vector<VeraMonitorInfo> Win32Backend::getMonitors() const {
    std::vector<VeraMonitorInfo> out;
    EnumDisplayMonitors(nullptr, nullptr, enumMonitorProc,
                        reinterpret_cast<LPARAM>(&out));
    return out;
}

VeraMonitorInfo Win32Backend::getPrimaryMonitor() const {
    for (auto& m : getMonitors()) {
        if (m.isPrimary) return m;
    }
    auto monitors = getMonitors();
    return monitors.empty() ? VeraMonitorInfo{} : monitors.front();
}

VeraMonitorInfo Win32Backend::getMonitorAt(int32_t x, int32_t y) const {
    POINT pt{x, y};
    HMONITOR hMonitor = MonitorFromPoint(pt, MONITOR_DEFAULTTOPRIMARY);
    MONITORINFOEXW info{};
    info.cbSize = sizeof(info);
    GetMonitorInfoW(hMonitor, &info);
    return toVeraMonitor(hMonitor, (info.dwFlags & MONITORINFOF_PRIMARY) != 0);
}

std::vector<VeraDisplayModeInfo> Win32Backend::getSupportedDisplayModes(
    const VeraMonitorInfo& monitor) const {
    std::vector<VeraDisplayModeInfo> out;
    std::wstring deviceName = vera::platform::win32::utf8_to_wide(monitor.name);

    DEVMODEW devMode{};
    devMode.dmSize = sizeof(devMode);
    for (DWORD i = 0; EnumDisplaySettingsW(deviceName.c_str(), i, &devMode);
         ++i) {
        out.push_back(VeraDisplayModeInfo{
            .width = devMode.dmPelsWidth,
            .height = devMode.dmPelsHeight,
            .refreshRateHz = devMode.dmDisplayFrequency,
            .bitsPerPixel = devMode.dmBitsPerPel,
        });
    }
    return out;
}

bool Win32Backend::supportsNativeDecorationHitTesting() const { return true; }

std::string Win32Backend::getClipboardText() const {
    if (!OpenClipboard(nullptr)) return {};
    std::string result;
    if (HANDLE data = GetClipboardData(CF_UNICODETEXT)) {
        if (auto* wtext = static_cast<wchar_t*>(GlobalLock(data))) {
            result = vera::platform::win32::wide_to_utf8(wtext);
            GlobalUnlock(data);
        }
    }
    CloseClipboard();
    return result;
}

void Win32Backend::setClipboardText(const std::string& text) {
    std::wstring wide = vera::platform::win32::utf8_to_wide(text);
    if (!OpenClipboard(nullptr)) return;
    EmptyClipboard();

    size_t bytes = (wide.size() + 1) * sizeof(wchar_t);
    if (HGLOBAL mem = GlobalAlloc(GMEM_MOVEABLE, bytes)) {
        if (void* dst = GlobalLock(mem)) {
            memcpy(dst, wide.c_str(), bytes);
            GlobalUnlock(mem);
            SetClipboardData(CF_UNICODETEXT, mem);
        }
    }
    CloseClipboard();
}

bool Win32Backend::hasClipboardText() const {
    return IsClipboardFormatAvailable(CF_UNICODETEXT) != 0;
}

void Win32Backend::setDragCallback(VeraDragCallback callback) {
    m_dragCallback = std::move(callback);
    // TODO: hook DragAcceptFiles + WM_DROPFILES in
    // VeraWin32Window::handleMessage and invoke m_dragCallback from there, same
    // shape as X11DragDrop/wl_data_device.
}

VeraSystemTheme Win32Backend::getSystemTheme() const {
    // TODO: read HKCU\...\Personalize\AppsUseLightTheme via the registry.
    return VeraSystemTheme::Unknown;
}

std::vector<VeraInputDeviceInfo> Win32Backend::getInputDevices() const {
    return {};
}

VeraNativeHandle Win32Backend::getNativeHandle() const { return {}; }
