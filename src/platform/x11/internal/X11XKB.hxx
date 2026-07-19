#pragma once

#include "core/app/Types.h"
#include "platform/x11/internal/X11Internal.hxx"

void initializeXKBX11(X11Context& ctx);

VeraKey convertKeycodeToVeraKeyX11(X11Context& ctx, unsigned int keycode);

uint32_t convertKeyEventToCodepointX11(X11Context& ctx, XKeyEvent& event);
