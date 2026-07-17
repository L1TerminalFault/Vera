#pragma once

#include "core/input/Joystick.h"
#include "platform/wayland/internal/WaylandInternal.hxx"

namespace waylandjoystick {

void setButtonCallback(std::function<void(uint32_t, uint32_t, bool)> cb);
void setAxisCallback(std::function<void(uint32_t, uint32_t, float)> cb);

void initialize(WaylandContext& ctx);

void update(WaylandContext& ctx);

VeraJoystickState getState(uint32_t joystickId);

void shutdown(WaylandContext& ctx);

}  // namespace waylandjoystick
