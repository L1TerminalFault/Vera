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

        // Standard Unity Launcher DBus URI format for desktop launchers
        std::string appUri = "application://" + sanitizeAppId(bopts.appId);

        // Build properties dictionary: a{sv} (array of string -> variant)
        GVariantBuilder builder;
        g_variant_builder_init(&builder, G_VARIANT_TYPE("a{sv}"));

        // Add Badge Count properties
        g_variant_builder_add(
            &builder, "{sv}", "count",
            g_variant_new_int64(static_cast<gint64>(bopts.count)));
        g_variant_builder_add(
            &builder, "{sv}", "count-visible",
            g_variant_new_boolean(bopts.countVisible ? TRUE : FALSE));

        GVariant* properties = g_variant_builder_end(&builder);

        // Send D-Bus signal to desktop environment dock/launcher
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

        // Get session bus connection
        GDBusConnection* bus =
            g_bus_get_sync(G_BUS_TYPE_SESSION, nullptr, &error);
        if (!bus) {
            if (error) g_error_free(error);
            return false;
        }

        // Emit the Update signal defined by com.canonical.Unity.LauncherEntry
        gboolean success = g_dbus_connection_emit_signal(
            bus,
            nullptr,                               // Destination bus name
            "/com/canonical/unity/launcherentry",  // Object path
            "com.canonical.Unity.LauncherEntry",   // Interface name
            "Update",                              // Signal name

            // Fix: Add '@' before the container type to safely pass
            // 'properties'
            g_variant_new("(s@a{sv})", appUri, properties), &error);

        if (error) {
            g_error_free(error);
        }

        g_object_unref(bus);
        return success != 0;
    }

    // Ensures desktop ID extension matches expectations (.desktop)
    static std::string sanitizeAppId(const std::string& input) {
        if (input.size() < 8 || input.substr(input.size() - 8) != ".desktop") {
            return input + ".desktop";
        }
        return input;
    }
};
