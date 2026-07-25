#pragma once

#include "platform/wayland/internal/WaylandInternal.hxx"

void initializeDecorationWayland(WaylandContext& ctx);

zxdg_toplevel_decoration_v1* createDecorationWayland(WaylandContext& ctx,
                                                     xdg_toplevel* toplevel);

void destroyDecorationWayland(zxdg_toplevel_decoration_v1* decoration);
