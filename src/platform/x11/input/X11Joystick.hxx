#pragma once

#include "core/input/Joystick.h"
#include "platform/x11/internal/X11Internal.hxx"

namespace vera::x11::input {

void setButtonCallback(std::function<void(uint32_t, uint32_t, bool)> cb);

void setAxisCallback(std::function<void(uint32_t, uint32_t, float)> cb);

void initialize(internal::X11Context& ctx);

void update(internal::X11Context& ctx);

core::input::VeraJoystickState getState(uint32_t joystickId);

void shutdown(internal::X11Context& ctx);

}  // namespace vera::x11::input
