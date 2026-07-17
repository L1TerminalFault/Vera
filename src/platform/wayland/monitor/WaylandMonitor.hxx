#pragma once
#include <vector>

#include "core/monitor/Monitor.h"
#include "platform/wayland/internal/WaylandInternal.hxx"

namespace monitor {

std::vector<VeraMonitorInfo> getMonitors(const WaylandContext& ctx);

VeraMonitorInfo getPrimaryMonitor(const WaylandContext& ctx);

VeraMonitorInfo getMonitorAt(const WaylandContext& ctx, int32_t x, int32_t y);

std::vector<VeraDisplayModeInfo> getSupportedDisplayModes(
    const WaylandContext& ctx, const VeraMonitorInfo& monitor);

}  // namespace monitor
