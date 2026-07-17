#pragma once
#include <cstdint>
#include <optional>
#include <string>

struct VeraRect {
    uint32_t x = 0;
    uint32_t y = 0;
    uint32_t width = 0;
    uint32_t height = 0;
};

struct VeraWindowState {
    uint32_t width, height;
    int32_t x, y;
    bool isMinimized, isMaximized, isFullscreen, isFocused, isVisible;
};

enum class FullScreenMode { Windowed, Borderless, Exclusive };

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

struct VeraWindowInfo {
    std::optional<uint32_t> x;
    std::optional<uint32_t> y;

    uint32_t width;
    uint32_t height;

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

template <>
struct std::hash<VeraWindowHandle> {
    size_t operator()(const VeraWindowHandle& h) const noexcept {
        return std::hash<uint64_t>{}(h.value);
    }
};
