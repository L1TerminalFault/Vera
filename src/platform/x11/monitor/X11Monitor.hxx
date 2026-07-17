#pragma once

#include <X11/Xlib.h>

#include <vector>

#include "core/monitor/Monitor.h"
#include "platform/x11/internal/X11Internal.hxx"

namespace vera::x11::monitor {

bool initialize(internal::X11Context& ctx);

std::vector<core::monitor::VeraMonitorInfo> getMonitors(
    internal::X11Context& ctx);

core::monitor::VeraMonitorInfo getPrimaryMonitor(internal::X11Context& ctx);

core::monitor::VeraMonitorInfo getMonitorAt(internal::X11Context& ctx,
                                            int32_t x, int32_t y);

std::vector<core::monitor::VeraDisplayModeInfo> getSupportedDisplayModes(
    internal::X11Context& ctx, const core::monitor::VeraMonitorInfo& monitor);

}  // namespace vera::x11::monitor
