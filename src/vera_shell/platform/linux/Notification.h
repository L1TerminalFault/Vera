#pragma once

#include <gio/gio.h>  // Native GIO/DBus library

#include <cstdint>
#include <string>

#include "../../core_shell/Types.h"

class LinuxNotification {
   public:
    // -------------------------------------------------------------------------
    // Show or Update a Desktop Notification
    // -------------------------------------------------------------------------

    static NotificationResult showNotification(
        const NotificationOptions& nopts) {
        NotificationResult result;

        GError* error = nullptr;
        GDBusConnection* bus =
            g_bus_get_sync(G_BUS_TYPE_SESSION, nullptr, &error);
        if (!bus) {
            if (error) g_error_free(error);
            return result;
        }

        // 1. Build actions array: as (alternating key, label strings)
        GVariantBuilder actionsBuilder;
        g_variant_builder_init(&actionsBuilder, G_VARIANT_TYPE("as"));
        for (const auto& action : nopts.actions) {
            g_variant_builder_add(&actionsBuilder, "s", action.first.c_str());
            g_variant_builder_add(&actionsBuilder, "s", action.second.c_str());
        }
        // Fix 1: Finalize the actions array
        GVariant* actionsVariant = g_variant_builder_end(&actionsBuilder);

        // 2. Build hints dictionary: a{sv} (used for urgency, desktop-entry,
        // etc.)
        GVariantBuilder hintsBuilder;
        g_variant_builder_init(&hintsBuilder, G_VARIANT_TYPE("a{sv}"));
        g_variant_builder_add(
            &hintsBuilder, "{sv}", "urgency",
            g_variant_new_byte(static_cast<guchar>(nopts.urgency)));
        // Fix 2: Finalize the hints dictionary
        GVariant* hintsVariant = g_variant_builder_end(&hintsBuilder);

        // 3. Make D-Bus call to org.freedesktop.Notifications.Notify
        // Fix 3: Use '@as' and '@a{sv}' to feed the pre-built GVariant*
        // elements directly
        GVariant* reply = g_dbus_connection_call_sync(
            bus,
            "org.freedesktop.Notifications",   // Destination service
            "/org/freedesktop/Notifications",  // Object path
            "org.freedesktop.Notifications",   // Interface
            "Notify",                          // Method name
            g_variant_new("(susss@as@a{sv}i)",
                          nopts.appName.c_str(),  // app_name
                          nopts.replacesId,       // replaces_id
                          nopts.icon.c_str(),     // app_icon
                          nopts.title.c_str(),    // summary
                          nopts.body.c_str(),     // body
                          actionsVariant,         // actions (passed via @as)
                          hintsVariant,           // hints (passed via @a{sv})
                          nopts.expireTimeoutMs   // expire_timeout
                          ),
            G_VARIANT_TYPE(
                "(u)"),  // Expected return type: uint32 notification ID
            G_DBUS_CALL_FLAGS_NONE, -1, nullptr, &error);

        if (reply) {
            g_variant_get(reply, "(u)", &result.notificationId);
            result.success = (result.notificationId != 0);
            g_variant_unref(reply);
        } else {
            if (error) g_error_free(error);
        }

        g_object_unref(bus);
        return result;
    }

    // -------------------------------------------------------------------------
    // Close / Dismiss an Active Notification by ID
    // -------------------------------------------------------------------------

    static bool closeNotification(std::uint32_t id) {
        if (id == 0) return false;

        GError* error = nullptr;
        GDBusConnection* bus =
            g_bus_get_sync(G_BUS_TYPE_SESSION, nullptr, &error);
        if (!bus) {
            if (error) g_error_free(error);
            return false;
        }

        GVariant* reply = g_dbus_connection_call_sync(
            bus, "org.freedesktop.Notifications",
            "/org/freedesktop/Notifications", "org.freedesktop.Notifications",
            "CloseNotification", g_variant_new("(u)", id),
            nullptr,  // Method returns void
            G_DBUS_CALL_FLAGS_NONE, -1, nullptr, &error);

        bool success = (reply != nullptr);

        if (reply) {
            g_variant_unref(reply);
        } else if (error) {
            g_error_free(error);
        }

        g_object_unref(bus);
        return success;
    }
};
