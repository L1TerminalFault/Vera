#pragma once

#include "platform/x11/internal/X11Internal.hxx"

void setTitleX11(X11Context& ctx, Window window, const std::string& title);

void setIconX11(X11Context& ctx, Window window, const std::string& iconPath);

void setSizeHintsX11(X11Context& ctx, Window window, uint32_t minWidth,
                     uint32_t minHeight, uint32_t maxWidth, uint32_t maxHeight,
                     bool resizable);

void setNetWmStateX11(X11Context& ctx, Window window, Atom state1, Atom state2,
                      bool add);

void setAlwaysOnTopX11(X11Context& ctx, Window window, bool value);

void setWindowTypeX11(X11Context& ctx, Window window);

void setPidX11(X11Context& ctx, Window window);

bool hasNetWmStateX11(X11Context& ctx, Window window, Atom state);
