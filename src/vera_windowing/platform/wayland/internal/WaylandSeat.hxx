#pragma once

#include "platform/wayland/internal/WaylandInternal.hxx"

void bindSeatWayland(WaylandContext& ctx, wl_registry* registry, uint32_t name,
                     uint32_t version);
