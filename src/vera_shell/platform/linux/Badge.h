#pragma once

#include <gio/gio.h>  // Native GIO/DBus library

#include <string>

#include "../../core_shell/Types.h"

class LinuxBadge {
   public:
    static bool setBadge(const BadgeOptions& bopts) {
        if (bopts.appId.empty()) {
            return false;
        }

        // Must match your app's actual .desktop filename
        std::string sanitizedId = sanitizeAppId(bopts.appId);
        std::string appUri = "application://" + sanitizedId;

        // Build properties dictionary: a{sv} (array of string -> variant)
        GVariantBuilder builder;
        g_variant_builder_init(&builder, G_VARIANT_TYPE("a{sv}"));

        // 1. Core Unity Count fields
        g_variant_builder_add(
            &builder, "{sv}", "count",
            g_variant_new_int64(static_cast<gint64>(bopts.count)));
        g_variant_builder_add(
            &builder, "{sv}", "count-visible",
            g_variant_new_boolean(bopts.countVisible && bopts.count > 0));

        // 2. Add urgency flag (triggers dock bounces/highlights in KDE & GNOME)
        g_variant_builder_add(
            &builder, "{sv}", "urgent",
            g_variant_new_boolean(
                (bopts.countVisible && bopts.count > 0) ? TRUE : FALSE));

        GVariant* properties = g_variant_builder_end(&builder);

        return emitDbusSignal(appUri.c_str(), properties);
    }

    static bool clearBadge(const std::string& appId) {
        if (appId.empty()) return false;

        BadgeOptions clearOpts;
        clearOpts.appId = appId;
        clearOpts.count = 0;
        clearOpts.countVisible = false;

        return setBadge(clearOpts);
    }

   private:
    static bool emitDbusSignal(const char* appUri, GVariant* properties) {
        GError* error = nullptr;

        GDBusConnection* bus =
            g_bus_get_sync(G_BUS_TYPE_SESSION, nullptr, &error);
        if (!bus) {
            if (error) g_error_free(error);
            return false;
        }

        // g_dbus_connection_emit_signal consumes floating references.
        // We wrap parameters explicitly.
        GVariant* signalBody = g_variant_new("(s@a{sv})", appUri, properties);

        gboolean success = g_dbus_connection_emit_signal(
            bus,
            nullptr,  // Destination (null = broadcast signal)
            "/com/canonical/unity/launcherentry",  // Object path
            "com.canonical.Unity.LauncherEntry",   // Interface name
            "Update",                              // Signal name
            signalBody, &error);

        if (error) {
            g_error_free(error);
        }

        g_object_unref(bus);
        return success != 0;
    }

    static std::string sanitizeAppId(const std::string& input) {
        if (input.size() < 8 || input.substr(input.size() - 8) != ".desktop") {
            return input + ".desktop";
        }
        return input;
    }
};
