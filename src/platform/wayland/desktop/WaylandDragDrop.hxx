#pragma once

#include "core/window/DragDrop.h"
#include "platform/wayland/internal/WaylandInternal.hxx"

void initializeDnDWayland(WaylandContext& ctx);

void setDragCallbackWayland(WaylandContext& ctx, VeraDragCallback callback);

void shutdownDnDWayland();
