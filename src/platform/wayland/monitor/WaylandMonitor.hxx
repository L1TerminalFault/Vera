#pragma once

#include "core/monitor/Monitor.h"
#include "platform/wayland/internal/WaylandInternal.hxx"

std::vector<VeraMonitorInfo> getMonitorsWayland(const WaylandContext& ctx);

VeraMonitorInfo getPrimaryMonitorWayland(const WaylandContext& ctx);

VeraMonitorInfo getMonitorAtCoordinateXYWayland(const WaylandContext& ctx,
                                                int32_t x, int32_t y);

std::vector<VeraDisplayModeInfo> getSupportedDisplayModesWayland(
    const WaylandContext& ctx, const VeraMonitorInfo& monitor);
