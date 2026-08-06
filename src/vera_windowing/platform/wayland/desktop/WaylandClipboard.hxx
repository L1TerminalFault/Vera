#pragma once

#include "core/app/Types.h"
#include "platform/wayland/internal/WaylandInternal.hxx"

void initializeClipboardWayland(WaylandContext& ctx);

VeraStringView getClipboardTextWayland(WaylandContext& ctx);

void setClipboardTextWayland(WaylandContext& ctx, const std::string& text);

bool hasClipboardTextWayland(const WaylandContext& ctx);
