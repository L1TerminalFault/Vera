#pragma once

#include "core/app/Types.h"
#include "platform/x11/internal/X11Internal.hxx"

void setJoystickButtonCallbackX11(
    std::function<void(uint32_t, uint32_t, bool)> cb);

void setJoystickAxisCallbackX11(
    std::function<void(uint32_t, uint32_t, float)> cb);

void initializeJoystickX11(X11Context& ctx);

void updateJoystickX11(X11Context& ctx);

VeraJoystickState getJoystickStateX11(X11Context& ctx, uint32_t joystickId);

void shutdownJoystickX11(X11Context& ctx);
