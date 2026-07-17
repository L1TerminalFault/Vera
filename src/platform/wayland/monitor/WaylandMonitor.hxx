#pragma once
#include <vector>

#include "core/platform/IPlatformBackend.h"
#include "platform/wayland/internal/WaylandInternal.hxx"

namespace vera::wayland::monitor {

std::vector<VeraMonitorInfo> getMonitors(const internal::WaylandContext& ctx);

VeraMonitorInfo getPrimaryMonitor(const internal::WaylandContext& ctx);

VeraMonitorInfo getMonitorAt(const internal::WaylandContext& ctx, int32_t x,
                             int32_t y);

std::vector<VeraDisplayModeInfo> getSupportedDisplayModes(
    const internal::WaylandContext& ctx, const VeraMonitorInfo& monitor);

}  // namespace vera::wayland::monitor
