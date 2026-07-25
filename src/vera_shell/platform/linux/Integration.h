#pragma once

#include <gio/gio.h>  // Native GIO/DBus library

#include <string>

#include "../../core_shell/Types.h"

class LinuxIntegration {
   public:
    // -------------------------------------------------------------------------
    // Set / Clear Progress Bar on Dock Icon (via D-Bus)
    // -------------------------------------------------------------------------

    static bool setProgress(const ProgressOptions& popts) {
        if (popts.appId.empty()) {
            return false;
        }

        std::string appUri = "application://" + sanitizeAppId(popts.appId);

        // Build properties dictionary: a{sv} (array of string -> variant)
        GVariantBuilder builder;
        g_variant_builder_init(&builder, G_VARIANT_TYPE("a{sv}"));

        // Progress value (0.0 to 1.0)
        g_variant_builder_add(&builder, "{sv}", "progress",
                              g_variant_new_double(popts.progress));

        // Progress visibility flag
        g_variant_builder_add(
            &builder, "{sv}", "progress-visible",
            g_variant_new_boolean(popts.progressVisible ? TRUE : FALSE));

        GVariant* properties = g_variant_builder_end(&builder);

        return emitDbusSignal(appUri.c_str(), properties);
    }

    static bool clearProgress(const std::string& appId) {
        if (appId.empty()) return false;

        ProgressOptions clearOpts;
        clearOpts.appId = appId;
        clearOpts.progress = 0.0;
        clearOpts.progressVisible = false;

        return setProgress(clearOpts);
    }

    // -------------------------------------------------------------------------
    // Request Window / Application Attention (Flash Dock Icon or Urgent Flag)
    // -------------------------------------------------------------------------

    static bool requestAttention(AttentionType atype,
                                 const std::string& appId) {
        if (appId.empty()) return false;

        std::string appUri = "application://" + sanitizeAppId(appId);

        GVariantBuilder builder;
        g_variant_builder_init(&builder, G_VARIANT_TYPE("a{sv}"));

        // Sets the 'urgent' state on the launcher entry
        g_variant_builder_add(&builder, "{sv}", "urgent",
                              g_variant_new_boolean(TRUE));

        // Critical vs Informational hint handling
        if (atype == AttentionType::Critical) {
            // Re-trigger / hold urgency pulse on docks that support it
            g_variant_builder_add(&builder, "{sv}", "urgent-critical",
                                  g_variant_new_boolean(TRUE));
        }

        GVariant* properties = g_variant_builder_end(&builder);

        return emitDbusSignal(appUri.c_str(), properties);
    }

   private:
    static bool emitDbusSignal(const char* appUri, GVariant* properties) {
        GError* error = nullptr;

        GDBusConnection* bus =
            g_bus_get_sync(G_BUS_TYPE_SESSION, nullptr, &error);
        if (!bus) {
            if (error) g_error_free(error);
            // Free the properties variant if we can't get the bus to prevent a
            // leak
            if (properties) g_variant_unref(properties);
            return false;
        }

        // Fix: Use '@a{sv}' to correctly and safely link your pre-built
        // 'properties' variant
        GVariant* parameters = g_variant_new("(s@a{sv})", appUri, properties);

        gboolean success = g_dbus_connection_emit_signal(
            bus,
            nullptr,                               // Destination (broadcast)
            "/com/canonical/unity/launcherentry",  // Object path
            "com.canonical.Unity.LauncherEntry",   // Interface
            "Update",                              // Signal name
            parameters,  // Safely passed combined parameters
            &error);

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
