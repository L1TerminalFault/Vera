#pragma once

#include "platform/wayland/internal/WaylandInternal.hxx"

void unlockPointer(WaylandContext& ctx);

void lockPointer(WaylandContext& ctx, wl_surface* surface);
