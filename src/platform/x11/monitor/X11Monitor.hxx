#pragma once

#include <X11/Xlib.h>

#include <vector>

#include "core/monitor/Monitor.h"
#include "platform/x11/internal/X11Internal.hxx"

namespace monitor {

bool initialize(X11Context& ctx);

std::vector<VeraMonitorInfo> getMonitors(X11Context& ctx);

VeraMonitorInfo getPrimaryMonitor(X11Context& ctx);

VeraMonitorInfo getMonitorAt(X11Context& ctx, int32_t x, int32_t y);

std::vector<VeraDisplayModeInfo> getSupportedDisplayModes(
    X11Context& ctx, const VeraMonitorInfo& monitor);

}  // namespace monitor
