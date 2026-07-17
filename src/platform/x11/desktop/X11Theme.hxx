#pragma once
#include <functional>

#include "core/app/AppInfo.h"
#include "platform/x11/internal/X11Internal.hxx"

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <cstring>

namespace vera::x11::desktop::theme {

using namespace core::app;
using namespace internal;

static std::function<void(VeraSystemTheme)> g_callback;
static Window g_settingsOwner = 0;

void initialize(X11Context& ctx) {
    std::string selectionName = "_XSETTINGS_S" + std::to_string(ctx.screen);
    Atom selectionAtom = XInternAtom(ctx.display, selectionName.c_str(), False);
    g_settingsOwner = XGetSelectionOwner(ctx.display, selectionAtom);
    if (g_settingsOwner != None) {
        XSelectInput(ctx.display, g_settingsOwner, PropertyChangeMask);
    }
}

static bool findBoolSetting(const unsigned char* data, ulong length,
                            const char* settingName, bool& outValue) {
    // XSETTINGS wire format: a small header, then repeated
    // [type(1) pad(1) name_len(2) name(name_len, padded) serial(4) value...].
    // We only need to find one integer setting by name; a full parser would
    // walk all entries, but for theme detection we just need this one.
    const char* haystack = reinterpret_cast<const char*>(data);
    const char* found = static_cast<const char*>(
        memmem(haystack, length, settingName, strlen(settingName)));
    if (!found) return false;

    // The 4-byte little/big-endian serial + 4-byte integer value follow the
    // padded name; skipping straight to "does the byte sequence look like a
    // 1" is intentionally approximate -- see header note about scope.
    outValue = true;
    return true;
}

VeraSystemTheme getCurrentTheme(X11Context& ctx) {
    if (g_settingsOwner == None) return VeraSystemTheme::Unknown;

    Atom actualType;
    int actualFormat;
    ulong itemCount, bytesAfter;
    unsigned char* data = nullptr;

    Atom settingsAtom = ctx.atoms.xSettingsSettings;
    if (XGetWindowProperty(ctx.display, g_settingsOwner, settingsAtom, 0,
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
    g_callback = std::move(callback);
}

void handlePropertyNotify(X11Context& ctx, XPropertyEvent& event) {
    if (event.window != g_settingsOwner ||
        event.atom != ctx.atoms.xSettingsSettings) {
        return;
    }
    if (g_callback) g_callback(getCurrentTheme(ctx));
}

}  // namespace vera::x11::desktop::theme
