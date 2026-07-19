#pragma once

#include "core/app/Types.h"
#include "platform/wayland/internal/WaylandInternal.hxx"

void initializeDnDWayland(WaylandContext& ctx);

void setDragCallbackWayland(WaylandContext& ctx, VeraDragCallback callback);

void shutdownDnDWayland();
