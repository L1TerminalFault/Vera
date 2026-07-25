#pragma once

#include "core/app/Types.h"
#include "platform/wayland/internal/WaylandInternal.hxx"

void setCursorShapeWayland(WaylandContext& ctx, VeraCursorShape shape);

void setCursorModeWayland(WaylandContext& ctx, wl_surface* surface,
                          VeraCursorMode mode);
