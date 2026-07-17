#pragma once

#include "core/input/Joystick.h"
#include "platform/x11/internal/X11Internal.hxx"

namespace x11joystick {

void setButtonCallback(std::function<void(uint32_t, uint32_t, bool)> cb);

void setAxisCallback(std::function<void(uint32_t, uint32_t, float)> cb);

void initialize(X11Context& ctx);

void update(X11Context& ctx);

VeraJoystickState getState(uint32_t joystickId);

void shutdown(X11Context& ctx);

}  // namespace x11joystick
