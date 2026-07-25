#pragma once

#include "platform/wayland/internal/WaylandInternal.hxx"

void unlockPointerWayland(WaylandContext& ctx);

void lockPointerWayland(WaylandContext& ctx, wl_surface* surface);
