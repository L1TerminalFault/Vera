#include "platform/wayland/monitor/WaylandMonitor.hxx"

#include <vector>

#include "platform/wayland/internal/WaylandInternal.hxx"

namespace monitor {

std::vector<VeraMonitorInfo> getMonitors(const WaylandContext& ctx) {
    std::vector<VeraMonitorInfo> list;
    list.reserve(ctx.outputs.size());

    for (const auto& out : ctx.outputs) {
        VeraMonitorInfo info{};
        info.name = out.name.empty() ? "Generic Wayland Display" : out.name;

        info.x = out.x;
        info.y = out.y;

        info.workAreaX = out.x;
        info.workAreaY = out.y;
        info.workAreaWidth = (out.widthMm > 0) ? out.widthMm : 1920;
        info.workAreaHeight = (out.heightMm > 0) ? out.heightMm : 1080;

        info.dpiScale = static_cast<float>(out.scaleFactor);

        info.refreshRateHz = static_cast<uint32_t>(out.refreshRate / 1000);
        info.isPrimary = out.isPrimary;

        if (out.widthMm > 0 && out.heightMm > 0) {
            info.physicalWidthMm = static_cast<uint32_t>(out.widthMm);
            info.physicalHeightMm = static_cast<uint32_t>(out.heightMm);
        }

        list.push_back(info);
    }

    return list;
}

VeraMonitorInfo getPrimaryMonitor(const WaylandContext& ctx) {
    for (const auto& out : ctx.outputs) {
        if (out.isPrimary) {
            auto monitors = getMonitors(ctx);
            for (const auto& m : monitors) {
                if (m.name == out.name) return m;
            }
        }
    }

    auto monitors = getMonitors(ctx);
    if (!monitors.empty()) {
        return monitors.front();
    }

    return VeraMonitorInfo{};
}

VeraMonitorInfo getMonitorAt(const WaylandContext& ctx, int32_t x, int32_t y) {
    auto monitors = getMonitors(ctx);
    if (monitors.empty()) return VeraMonitorInfo{};

    for (const auto& monitor : monitors) {
        if (x >= monitor.x &&
            x < (monitor.x + static_cast<int32_t>(monitor.workAreaWidth)) &&
            y >= monitor.y &&
            y < (monitor.y + static_cast<int32_t>(monitor.workAreaHeight))) {
            return monitor;
        }
    }

    return monitors.front();
}

std::vector<VeraDisplayModeInfo> getSupportedDisplayModes(
    const WaylandContext& ctx, const VeraMonitorInfo& monitor) {
    std::vector<VeraDisplayModeInfo> modes;

    for (const auto& out : ctx.outputs) {
        if (out.name == monitor.name) {
            VeraDisplayModeInfo mode{};
            mode.width = monitor.workAreaWidth;
            mode.height = monitor.workAreaHeight;
            mode.refreshRateHz = static_cast<uint32_t>(out.refreshRate / 1000);
            mode.bitsPerPixel = 32;
            modes.push_back(mode);
            break;
        }
    }

    return modes;
}

}  // namespace monitor
