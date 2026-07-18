#pragma once

#include <vector>

#include "core/monitor/Monitor.h"
#include "platform/x11/internal/X11Internal.hxx"

bool initializeMonitorX11(X11Context& ctx);

std::vector<VeraMonitorInfo> getMonitorsX11(X11Context& ctx);

VeraMonitorInfo getPrimaryMonitorX11(X11Context& ctx);

VeraMonitorInfo getMonitorAtCoordinateXYX11(X11Context& ctx, int32_t x,
                                            int32_t y);

std::vector<VeraDisplayModeInfo> getSupportedDisplayModesX11(
    X11Context& ctx, const VeraMonitorInfo& monitor);
