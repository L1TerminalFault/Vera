#pragma once

#include <gio/gio.h>  // Native GIO/DBus library

#include <cstdint>
#include <string>
#include <unordered_map>

#include "../../core_shell/Types.h"

class LinuxPower {
   public:
    // -------------------------------------------------------------------------
    // Prevent System / Screen Sleep (Inhibit)
    // -------------------------------------------------------------------------

    static bool preventSleep(const SleepRequest& slreq) {
        GError* error = nullptr;
        GDBusConnection* bus =
            g_bus_get_sync(G_BUS_TYPE_SESSION, nullptr, &error);
        if (!bus) {
            if (error) g_error_free(error);
            return false;
        }

        std::uint32_t flags = translateTargetToFlags(slreq.target);
        std::uint32_t cookie = 0;

        // 1. Try XDG Desktop Portal Inhibit API (Modern, Flatpak/Wayland-safe)
        GVariantBuilder optBuilder;
        g_variant_builder_init(&optBuilder, G_VARIANT_TYPE("a{sv}"));
        g_variant_builder_add(&optBuilder, "{sv}", "reason",
                              g_variant_new_string(slreq.reason.c_str()));

        // Crucial Fix 1: Properly close the builder to get a safe, valid array
        // variant
        GVariant* options = g_variant_builder_end(&optBuilder);

        // Crucial Fix 2: Use the '@a{sv}' format specifier to accept the raw
        // GVariant* pointer
        GVariant* reply = g_dbus_connection_call_sync(
            bus, "org.freedesktop.portal.Desktop",
            "/org/freedesktop/portal/desktop", "org.freedesktop.portal.Inhibit",
            "Inhibit", g_variant_new("(su@a{sv})", "", flags, options),
            G_VARIANT_TYPE("(o)"), G_DBUS_CALL_FLAGS_NONE, -1, nullptr, &error);

        if (reply) {
            // Portal return path saved as active lock key
            g_variant_unref(reply);
            g_object_unref(bus);
            getCookieMap()[slreq.target] = 1;  // Mark active lock
            return true;
        }

        if (error) {
            g_error_free(error);
            error = nullptr;
        }

        // 2. Fallback to freedesktop ScreenSaver / PowerManager D-Bus API
        reply = g_dbus_connection_call_sync(
            bus, "org.freedesktop.ScreenSaver", "/org/freedesktop/ScreenSaver",
            "org.freedesktop.ScreenSaver", "Inhibit",
            g_variant_new("(ss)", "App", slreq.reason.c_str()),
            G_VARIANT_TYPE("(u)"), G_DBUS_CALL_FLAGS_NONE, -1, nullptr, &error);

        if (reply) {
            g_variant_get(reply, "(u)", &cookie);
            g_variant_unref(reply);
            getCookieMap()[slreq.target] = cookie;
            g_object_unref(bus);
            return true;
        }

        if (error) {
            g_error_free(error);
        }

        g_object_unref(bus);
        return false;
    }

    // -------------------------------------------------------------------------
    // Allow System / Screen Sleep (Uninhibit)
    // -------------------------------------------------------------------------

    static bool allowSleep(SleepTarget sltgt) {
        auto& activeCookies = getCookieMap();
        auto it = activeCookies.find(sltgt);

        if (it == activeCookies.end() || it->second == 0) {
            return false;  // No active sleep inhibition found for target
        }

        std::uint32_t cookie = it->second;

        GError* error = nullptr;
        GDBusConnection* bus =
            g_bus_get_sync(G_BUS_TYPE_SESSION, nullptr, &error);
        if (!bus) {
            if (error) g_error_free(error);
            return false;
        }

        // Call Uninhibit on ScreenSaver service
        GVariant* reply = g_dbus_connection_call_sync(
            bus, "org.freedesktop.ScreenSaver", "/org/freedesktop/ScreenSaver",
            "org.freedesktop.ScreenSaver", "UnInhibit",
            g_variant_new("(u)", cookie), nullptr, G_DBUS_CALL_FLAGS_NONE, -1,
            nullptr, &error);

        bool success = (reply != nullptr);

        if (reply) {
            g_variant_unref(reply);
        } else if (error) {
            g_error_free(error);
        }

        g_object_unref(bus);
        activeCookies.erase(it);
        return success;
    }

   private:
    static std::unordered_map<SleepTarget, std::uint32_t>& getCookieMap() {
        static std::unordered_map<SleepTarget, std::uint32_t> cookieMap;
        return cookieMap;
    }

    static std::uint32_t translateTargetToFlags(SleepTarget target) {
        // XDG Inhibit Flags: 1 = Logout, 2 = Switch User, 4 = Suspend, 8 =
        // Idle/Screen
        switch (target) {
            case SleepTarget::Screen:
                return 8;  // Idle / Display dimming
            case SleepTarget::System:
                return 4;  // Suspend / Sleep
            case SleepTarget::All:
                return 4 | 8;  // Both suspend and display idle
            default:
                return 4;
        }
    }
};
