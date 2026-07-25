#pragma once

#include <functional>

#include "core/app/Types.h"
#include "platform/x11/internal/X11Internal.hxx"

void initializeThemeX11(X11Context& ctx);

VeraSystemTheme getCurrentThemeX11(X11Context& ctx);

void setThemeChangeCallbackX11(std::function<void(VeraSystemTheme)> callback);

void handleThemePropertyNotifyX11(X11Context& ctx, XPropertyEvent& event);
