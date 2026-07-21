#include "X11Monitor.hxx"

#include "platform/x11/monitor/X11RandR.hxx"

static bool gHasRandR = false;

bool initializeMonitorX11(X11Context& ctx) {
    gHasRandR = initializeXRandRX11(ctx);
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

std::vector<VeraMonitorInfo> getMonitorsX11(X11Context& ctx) {
    if (gHasRandR) {
        auto monitors = queryMonitorsX11(ctx);
        if (!monitors.empty()) return monitors;
    }
    return {fallbackWholeScreen(ctx)};
}

VeraMonitorInfo getPrimaryMonitorX11(X11Context& ctx) {
    for (auto& m : getMonitorsX11(ctx)) {
        if (m.isPrimary) return m;
    }
    auto monitors = getMonitorsX11(ctx);
    return monitors.empty() ? fallbackWholeScreen(ctx) : monitors.front();
}

VeraMonitorInfo getMonitorAtCoordinateXYX11(X11Context& ctx, int32_t x,
                                            int32_t y) {
    for (auto& m : getMonitorsX11(ctx)) {
        bool inX = x >= m.x && x < m.x + static_cast<int32_t>(m.workAreaWidth);
        bool inY = y >= m.y && y < m.y + static_cast<int32_t>(m.workAreaHeight);
        if (inX && inY) return m;
    }
    return getPrimaryMonitorX11(ctx);
}

std::vector<VeraDisplayModeInfo> getSupportedDisplayModesX11(
    X11Context& ctx, const VeraMonitorInfo& monitor) {
    if (!gHasRandR) return {};
    return queryDisplayModesX11(ctx, monitor);
}
