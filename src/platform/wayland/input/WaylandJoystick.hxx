#pragma once

#include "core/input/Joystick.h"
#include "platform/wayland/internal/WaylandInternal.hxx"

void setJoystickButtonCallbackWayland(
    std::function<void(uint32_t, uint32_t, bool)> cb);

void setJoystickAxisCallbackWayland(
    std::function<void(uint32_t, uint32_t, float)> cb);

void initializeJoystickWayland(WaylandContext& ctx);

void updateJoystickWayland(WaylandContext& ctx);

VeraJoystickState getStateJoystickWayland(uint32_t joystickId);

void shutdownJoystickWayland(WaylandContext& ctx);
