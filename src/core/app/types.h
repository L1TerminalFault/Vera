#pragma once
#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <vector>

enum class VeraLinuxProtocol : uint8_t {
    Auto = 0,
    Wayland,
    X11,
};

enum class VeraSystemTheme { Light, Dark, Unknown };

class VeraWindow;

struct VeraNativeHandle {
    void* hwnd = nullptr;
    void* display = nullptr;
    uint64_t x11Window = 0;
    void* waylandSurface = nullptr;
};

struct VeraRect {
    uint32_t x = 0;
    uint32_t y = 0;
    uint32_t width = 0;
    uint32_t height = 0;
};

struct VeraMonitorInfo {
    std::string name;
    int32_t x, y;
    int32_t workAreaX, workAreaY;
    uint32_t workAreaWidth, workAreaHeight;
    float dpiScale;
    uint32_t refreshRateHz;
    bool isPrimary;
    std::optional<uint32_t> physicalWidthMm, physicalHeightMm;
};

struct VeraDisplayModeInfo {
    uint32_t width, height;
    uint32_t refreshRateHz;
    uint32_t bitsPerPixel;
};

struct VeraInputDeviceInfo {
    std::string name;
    bool connected;
};

struct VeraWindowState {
    uint32_t width, height;
    int32_t x, y;
    bool isMinimized, isMaximized, isFullscreen, isFocused, isVisible;
};

struct VeraAppInfo {
    bool enablePlatformDebugging = false;
    VeraLinuxProtocol preferedLinuxProtocol = VeraLinuxProtocol::Auto;
};

enum class FullScreenMode { Windowed, Borderless, Exclusive };

struct VeraWindowInfo {
    // optional manual positioning of window
    std::optional<uint32_t> x;
    std::optional<uint32_t> y;
    // default sizing
    // Note: will be ignored on tiled window managers
    uint32_t width;
    uint32_t height;
    // sizing constraints
    uint32_t minWidth = 0, minHeight = 0;
    uint32_t maxWidth = 0, maxHeight = 0;

    std::string title = "Vera Window";

    bool resizable = true;
    bool decorated = true;

    bool startMaximized = false;
    bool startMinimized = false;
    FullScreenMode fullscreenMode = FullScreenMode::Windowed;

    bool startVisible = true;
    bool focusOnShow = true;
    bool alwaysOnTop = false;
    bool customTitleBar = false;
    uint32_t titleBarHeight = 32;

    int monitorIndex = 0;
    bool centerOnMonitor = true;
    bool transparentFramebuffer = false;
    std::string iconPath;
};

// for drag and drop of files
enum class VeraDragAction { Enter, Over, Drop, Leave };

struct VeraDragEvent {
    VeraDragAction action;
    VeraWindow* window;
    int32_t x, y;
    std::vector<std::string> paths;
};

using VeraDragCallback = std::function<bool(const VeraDragEvent&)>;

struct VeraHitTestRegions {
    std::optional<VeraRect> dragRegion;
    std::optional<VeraRect> minimizeButton;
    std::optional<VeraRect> maximizeButton;
    std::optional<VeraRect> closeButton;
};

struct VeraWindowHandle {
    uint64_t value = 0;

    bool operator==(const VeraWindowHandle&) const = default;
    bool operator!=(const VeraWindowHandle&) const = default;
    explicit operator bool() const { return value != 0; }

    static constexpr VeraWindowHandle invalid() { return VeraWindowHandle{0}; }
};

namespace std {
template <>
struct hash<VeraWindowHandle> {
    size_t operator()(const VeraWindowHandle& h) const noexcept {
        return std::hash<uint64_t>{}(h.value);
    }
};
}  // namespace std
