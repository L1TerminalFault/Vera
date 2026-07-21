#pragma once

#include "core/app/Types.h"
#include "platform/wayland/internal/WaylandInternal.hxx"

void initializeThemeWayland(WaylandContext& ctx);

VeraThemeMode getActiveThemeModeWayland();

void updateThemeWayland();

void shutdownThemeWayland();
