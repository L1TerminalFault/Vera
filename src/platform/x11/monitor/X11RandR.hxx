#pragma once

#include <vector>

#include "core/monitor/Monitor.h"
#include "platform/x11/internal/X11Internal.hxx"

bool initializeXRandRX11(X11Context& ctx);

float queryDpiScaleX11(X11Context& ctx);

std::vector<VeraMonitorInfo> queryMonitorsX11(X11Context& ctx);

std::vector<VeraDisplayModeInfo> queryDisplayModesX11(
    X11Context& ctx, const VeraMonitorInfo& monitor);
