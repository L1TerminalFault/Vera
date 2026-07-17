#pragma once

#include <X11/XF86keysym.h>
#include <X11/XKBlib.h>
#include <X11/Xlib.h>

#include <cstdint>

#include "core/input/Keys.h"
#include "platform/x11/internal/X11Internal.hxx"

namespace xkb {

void initialize(X11Context& ctx);

VeraKey keycodeToVeraKey(X11Context& ctx, unsigned int keycode);

uint32_t keyEventToCodepoint(X11Context& ctx, XKeyEvent& event);

}  // namespace xkb
