#pragma once

#include "platform/x11/internal/X11Internal.hxx"

void initializeClipboardX11(X11Context& ctx);

void setClipboardTextX11(X11Context& ctx, const std::string& text);

std::string getClipboardTextX11(X11Context& ctx);

bool hasClipboardTextX11(X11Context& ctx);

void handleClipboardSelectionRequestX11(X11Context& ctx,
                                        XSelectionRequestEvent& request);

void handleClipboardSelectionClearX11(X11Context&, XSelectionClearEvent&);
