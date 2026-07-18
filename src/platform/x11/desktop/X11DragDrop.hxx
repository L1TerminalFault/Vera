#pragma once

#include "core/window/DragDrop.h"
#include "platform/x11/internal/X11Internal.hxx"

void setCallback(VeraDragCallback callback);

void handleClientMessage(X11Context& ctx, VeraWindow* window,
                         XClientMessageEvent& event);

void handleSelectionNotify(X11Context& ctx, VeraWindow* window,
                           XSelectionEvent& event);
