#pragma once

#include "platform/x11/internal/X11Internal.hxx"

#undef Status
#undef Success
#undef Bool

#include <X11/cursorfont.h>

#include "core/input/Mouse.h"

namespace vera::x11::cursor {

using namespace core::input;
using namespace internal;

void applyMode(X11Context& ctx, Window window, VeraCursorMode mode);

void shutdown(X11Context& ctx);

void applyShape(X11Context& ctx, Window window, VeraCursorShape shape);

}  // namespace vera::x11::cursor
