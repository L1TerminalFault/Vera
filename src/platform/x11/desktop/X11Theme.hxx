#pragma once
#include <functional>

#include "core/app/AppInfo.h"
#include "platform/x11/internal/X11Internal.hxx"

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <cstring>

namespace theme {

static std::function<void(VeraSystemTheme)> gCallback;
static Window gSettingsOwner = 0;

void initialize(X11Context& ctx) {
    std::string selectionName = "_XSETTINGS_S" + std::to_string(ctx.screen);
    Atom selectionAtom = XInternAtom(ctx.display, selectionName.c_str(), False);
    gSettingsOwner = XGetSelectionOwner(ctx.display, selectionAtom);
    if (gSettingsOwner != None) {
        XSelectInput(ctx.display, gSettingsOwner, PropertyChangeMask);
    }
}

static bool findBoolSetting(const unsigned char* data, ulong length,
                            const char* settingName, bool& outValue) {
    const char* haystack = reinterpret_cast<const char*>(data);
    const char* found = static_cast<const char*>(
        memmem(haystack, length, settingName, strlen(settingName)));
    if (!found) return false;

    outValue = true;
    return true;
}

VeraSystemTheme getCurrentTheme(X11Context& ctx) {
    if (gSettingsOwner == None) return VeraSystemTheme::Unknown;

    Atom actualType;
    int actualFormat;
    ulong itemCount, bytesAfter;
    unsigned char* data = nullptr;

    Atom settingsAtom = ctx.atoms.xSettingsSettings;
    if (XGetWindowProperty(ctx.display, gSettingsOwner, settingsAtom, 0,
                           1 << 16, False, AnyPropertyType, &actualType,
                           &actualFormat, &itemCount, &bytesAfter,
                           &data) != Success ||
        !data) {
        return VeraSystemTheme::Unknown;
    }

    VeraSystemTheme theme = VeraSystemTheme::Unknown;
    bool preferDark = false;
    if (findBoolSetting(data, itemCount, "Gtk/ApplicationPreferDarkTheme",
                        preferDark)) {
        theme = preferDark ? VeraSystemTheme::Dark : VeraSystemTheme::Light;
    }
    XFree(data);
    return theme;
}

void setChangeCallback(std::function<void(VeraSystemTheme)> callback) {
    gCallback = std::move(callback);
}

void handlePropertyNotify(X11Context& ctx, XPropertyEvent& event) {
    if (event.window != gSettingsOwner ||
        event.atom != ctx.atoms.xSettingsSettings) {
        return;
    }
    if (gCallback) gCallback(getCurrentTheme(ctx));
}

}  // namespace theme
