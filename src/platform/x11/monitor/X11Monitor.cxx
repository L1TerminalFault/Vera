#include "X11Monitor.hxx"

#include "platform/x11/monitor/X11RandR.hxx"

static bool gHasRandR = false;

namespace monitor {

bool initialize(X11Context& ctx) {
    gHasRandR = xrandr::initialize(ctx);
    return true;
}

static VeraMonitorInfo fallbackWholeScreen(X11Context& ctx) {
    VeraMonitorInfo info;
    info.name = "X11-Screen-0";
    info.x = 0;
    info.y = 0;
    info.workAreaX = 0;
    info.workAreaY = 0;
    info.workAreaWidth =
        static_cast<uint32_t>(DisplayWidth(ctx.display, ctx.screen));
    info.workAreaHeight =
        static_cast<uint32_t>(DisplayHeight(ctx.display, ctx.screen));
    info.dpiScale = 1.0f;
    info.refreshRateHz = 60;
    info.isPrimary = true;
    return info;
}

std::vector<VeraMonitorInfo> getMonitors(X11Context& ctx) {
    if (gHasRandR) {
        auto monitors = xrandr::queryMonitors(ctx);
        if (!monitors.empty()) return monitors;
    }
    return {fallbackWholeScreen(ctx)};
}

VeraMonitorInfo getPrimaryMonitor(X11Context& ctx) {
    for (auto& m : getMonitors(ctx)) {
        if (m.isPrimary) return m;
    }
    auto monitors = getMonitors(ctx);
    return monitors.empty() ? fallbackWholeScreen(ctx) : monitors.front();
}

VeraMonitorInfo getMonitorAt(X11Context& ctx, int32_t x, int32_t y) {
    for (auto& m : getMonitors(ctx)) {
        bool inX = x >= m.x && x < m.x + static_cast<int32_t>(m.workAreaWidth);
        bool inY = y >= m.y && y < m.y + static_cast<int32_t>(m.workAreaHeight);
        if (inX && inY) return m;
    }
    return getPrimaryMonitor(ctx);
}

std::vector<VeraDisplayModeInfo> getSupportedDisplayModes(
    X11Context& ctx, const VeraMonitorInfo& monitor) {
    if (!gHasRandR) return {};
    return xrandr::queryDisplayModes(ctx, monitor);
}

}  // namespace monitor
