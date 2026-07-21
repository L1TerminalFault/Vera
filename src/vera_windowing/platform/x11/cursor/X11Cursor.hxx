#pragma once

#include "platform/x11/internal/X11Internal.hxx"

#undef Status
#undef Success
#undef Bool

#include <X11/cursorfont.h>

#include "core/app/Types.h"

void applyCursorModeX11(X11Context& ctx, Window window, VeraCursorMode mode);

void shutdownCursorX11(X11Context& ctx);

void applyCursorShapeX11(X11Context& ctx, Window window, VeraCursorShape shape);
