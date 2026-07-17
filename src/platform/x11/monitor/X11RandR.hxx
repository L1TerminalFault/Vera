#pragma once

#include <vector>

#include "core/monitor/Monitor.h"
#include "platform/x11/internal/X11Internal.hxx"

namespace xrandr {

bool initialize(X11Context& ctx);

float queryDpiScale(X11Context& ctx);

std::vector<VeraMonitorInfo> queryMonitors(X11Context& ctx);

std::vector<VeraDisplayModeInfo> queryDisplayModes(
    X11Context& ctx, const VeraMonitorInfo& monitor);

}  // namespace xrandr
