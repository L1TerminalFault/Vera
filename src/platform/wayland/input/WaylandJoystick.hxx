#pragma once

#include "core/input/Joystick.h"
#include "platform/wayland/internal/WaylandInternal.hxx"

namespace vera::wayland::input {

void setButtonCallback(std::function<void(uint32_t, uint32_t, bool)> cb);
void setAxisCallback(std::function<void(uint32_t, uint32_t, float)> cb);

void initialize(internal::WaylandContext& ctx);

void update(internal::WaylandContext& ctx);

core::input::VeraJoystickState getState(uint32_t joystickId);

void shutdown(internal::WaylandContext& ctx);

}  // namespace vera::wayland::input
