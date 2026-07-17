#pragma once

#include <vector>

#include "core/monitor/Monitor.h"
#include "platform/x11/internal/X11Internal.hxx"

namespace vera::x11::monitor::xrandr {

bool initialize(internal::X11Context& ctx);

float queryDpiScale(internal::X11Context& ctx);

std::vector<core::monitor::VeraMonitorInfo> queryMonitors(
    internal::X11Context& ctx);

std::vector<core::monitor::VeraDisplayModeInfo> queryDisplayModes(
    internal::X11Context& ctx, const core::monitor::VeraMonitorInfo& monitor);

}  // namespace vera::x11::monitor::xrandr
