#pragma once

#include "platform/wayland/internal/WaylandInternal.hxx"

namespace vera::wayland::inputconstraints {

void unlockPointer(internal::WaylandContext& ctx);

void lockPointer(internal::WaylandContext& ctx, wl_surface* surface);

}  // namespace vera::wayland::inputconstraints
