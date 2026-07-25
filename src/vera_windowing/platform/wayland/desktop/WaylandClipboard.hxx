#pragma once

#include "platform/wayland/internal/WaylandInternal.hxx"

void initializeClipboardWayland(WaylandContext& ctx);

std::string getClipboardTextWayland(WaylandContext& ctx);

void setClipboardTextWayland(WaylandContext& ctx, const std::string& text);

bool hasClipboardTextWayland(const WaylandContext& ctx);
