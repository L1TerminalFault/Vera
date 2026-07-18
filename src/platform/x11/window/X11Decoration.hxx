#pragma once

#include "platform/x11/internal/X11Internal.hxx"

void setDecoratedX11(X11Context& ctx, Window window, bool decorated);

void handleTitlebarButtonPressX11(X11Context& ctx, Window window,
                                  const VeraHitTestRegions& regions,
                                  int32_t clickX, int32_t clickY);
