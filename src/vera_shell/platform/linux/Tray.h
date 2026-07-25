#pragma once

#include <gio/gio.h>
#include <unistd.h>

#include <condition_variable>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "../../core_shell/Types.h"

class LinuxTray {
   public:
    static bool createTray(const TrayOptions& topts) {
        std::lock_guard<std::mutex> lock(sMutex);
        if (sIsCreated) {
            removeTray();
        }

        sOptions = topts;
        bool threadReady = false;
        bool threadSuccess = false;
        std::mutex readyMutex;
        std::condition_variable readyCv;

        sWorkerThread = std::thread([&]() {
            GMainContext* privateContext = g_main_context_new();
            g_main_context_push_thread_default(privateContext);

            auto signalFailure = [&]() {
                {
                    std::lock_guard<std::mutex> readyLock(readyMutex);
                    threadReady = true;
                    threadSuccess = false;
                }
                readyCv.notify_one();
                g_main_context_pop_thread_default(privateContext);
                g_main_context_unref(privateContext);
            };

            GError* error = nullptr;
            sBus = g_bus_get_sync(G_BUS_TYPE_SESSION, nullptr, &error);
            if (!sBus) {
                if (error) {
                    g_printerr("[Tray Error] Bus failed: %s\n", error->message);
                    g_error_free(error);
                }
                signalFailure();
                return;
            }

            sServiceName =
                "org.kde.StatusNotifierItem-" + std::to_string(getpid()) + "-1";
            sObjectPath = "/StatusNotifierItem";

            g_bus_own_name_on_connection(sBus, sServiceName.c_str(),
                                         G_BUS_NAME_OWNER_FLAGS_NONE, nullptr,
                                         nullptr, nullptr, nullptr);

            static const char introspectionXml[] =
                "<node>"
                "  <interface name='org.kde.StatusNotifierItem'>"
                "    <property name='Category' type='s' access='read'/>"
                "    <property name='Id' type='s' access='read'/>"
                "    <property name='Title' type='s' access='read'/>"
                "    <property name='Status' type='s' access='read'/>"
                "    <property name='IconName' type='s' access='read'/>"
                "    <property name='ToolTip' type='(sa(iiay)ss)' access='read'/>"
                "    <signal name='NewTitle'/>"
                "    <signal name='NewIcon'/>"
                "    <signal name='NewStatus'/>"
                "    <signal name='NewToolTip'/>"
                "  </interface>"
                "</node>";

            error = nullptr;
            GDBusNodeInfo* nodeInfo =
                g_dbus_node_info_new_for_xml(introspectionXml, &error);
            if (error) {
                g_printerr("[Tray Error] XML Parse Error: %s\n",
                           error->message);
                g_error_free(error);
                signalFailure();
                return;
            }

            static const GDBusInterfaceVTable interfaceVtable = {
                [](GDBusConnection*, const char*, const char*, const char*,
                   const char*, GVariant*, GDBusMethodInvocation* invocation,
                   gpointer) {
                    g_dbus_method_invocation_return_value(invocation, nullptr);
                },
                [](GDBusConnection*, const char*, const char*, const char*,
                   const char* propertyName, GError**, gpointer) -> GVariant* {
                    std::lock_guard<std::mutex> propLock(sMutex);
                    std::string prop(propertyName);
                    if (prop == "Category")
                        return g_variant_new_string("ApplicationStatus");
                    if (prop == "Id")
                        return g_variant_new_string(sOptions.id.c_str());
                    if (prop == "Title")
                        return g_variant_new_string(sOptions.title.c_str());
                    if (prop == "Status") return g_variant_new_string("Active");
                    if (prop == "IconName")
                        return g_variant_new_string(sOptions.iconName.c_str());
                    if (prop == "ToolTip") {
    // Generates a valid (sa(iiay)ss) structure with an empty image byte array
    return g_variant_new(
        "(s@a(iiay)ss)", 
        sOptions.iconName.c_str(),                  // Icon Name
        g_variant_new_array(G_VARIANT_TYPE("(iiay)"), nullptr, 0), // Empty Icon Data
        sOptions.title.c_str(),                     // Tooltip Title
        sOptions.tooltip.c_str()                    // Tooltip Description
    );
}
return nullptr;
                },
                nullptr,
                {nullptr}};

            error = nullptr;
            sRegistrationId = g_dbus_connection_register_object(
                sBus, sObjectPath.c_str(), nodeInfo->interfaces[0],
                &interfaceVtable, nullptr, nullptr, &error);

            g_dbus_node_info_unref(nodeInfo);

            if (error) {
                g_printerr("[Tray Error] Registration failed: %s\n",
                           error->message);
                g_error_free(error);
                signalFailure();
                return;
            }

            error = nullptr;
            GVariant* reply = g_dbus_connection_call_sync(
                sBus, "org.kde.StatusNotifierWatcher", "/StatusNotifierWatcher",
                "org.kde.StatusNotifierWatcher", "RegisterStatusNotifierItem",
                g_variant_new("(s)", sServiceName.c_str()), nullptr,
                G_DBUS_CALL_FLAGS_NONE, -1, nullptr, &error);

            if (error) {
                g_printerr(
                    "[Tray Warning] Host Host Environment lacks a Tray "
                    "Watcher: %s\n",
                    error->message);
                g_error_free(error);
                // Non-fatal bypass: some desktop extensions attach items
                // retrospectively
            }
            if (reply) g_variant_unref(reply);

            sMainLoop = g_main_loop_new(privateContext, FALSE);

            {
                std::lock_guard<std::mutex> readyLock(readyMutex);
                threadReady = true;
                threadSuccess = true;
                sIsCreated = true;
            }
            readyCv.notify_one();

            g_main_loop_run(sMainLoop);

            g_main_loop_unref(sMainLoop);
            sMainLoop = nullptr;
            g_main_context_pop_thread_default(privateContext);
            g_main_context_unref(privateContext);
        });

        std::unique_lock<std::mutex> readyLock(readyMutex);
        readyCv.wait(readyLock, [&] { return threadReady; });

        return threadSuccess;
    }

    static bool updateTray(const TrayUpdate& tupdt) {
        std::lock_guard<std::mutex> lock(sMutex);
        if (!sIsCreated || !sBus) return false;

        if (!tupdt.title.empty()) {
            sOptions.title = tupdt.title;
            emitSignal("NewTitle");
        }
        if (!tupdt.iconName.empty()) {
            sOptions.iconName = tupdt.iconName;
            emitSignal("NewIcon");
        }
        if (!tupdt.tooltip.empty()) {
            sOptions.tooltip = tupdt.tooltip;
            emitSignal("NewToolTip");
        }
        if (!tupdt.menuItems.empty()) {
            sOptions.menuItems = tupdt.menuItems;
        }
        return true;
    }

    static bool removeTray() {
        std::lock_guard<std::mutex> lock(sMutex);
        if (!sIsCreated) return true;

        if (sMainLoop) {
            g_main_loop_quit(sMainLoop);
            // Don't unref here if it's managed by the worker thread loop exit
            // path
        }

        // Critically important: Wait for the thread execution to safely park
        // and stop
        if (sWorkerThread.joinable()) {
            sWorkerThread.join();
        }

        if (sBus && sRegistrationId > 0) {
            g_dbus_connection_unregister_object(sBus, sRegistrationId);
            sRegistrationId = 0;
        }

        if (sBus) {
            g_object_unref(sBus);
            sBus = nullptr;
        }

        sIsCreated = false;
        return true;
    }

   private:
    static void emitSignal(const char* signalName) {
        if (!sBus) return;
        g_dbus_connection_emit_signal(sBus, nullptr, sObjectPath.c_str(),
                                      "org.kde.StatusNotifierItem", signalName,
                                      nullptr, nullptr);
    }

    inline static bool sIsCreated = false;
    inline static GDBusConnection* sBus = nullptr;
    inline static GMainLoop* sMainLoop = nullptr;
    inline static guint sRegistrationId = 0;
    inline static std::string sServiceName;
    inline static std::string sObjectPath;
    inline static TrayOptions sOptions;
    inline static std::thread sWorkerThread;
    inline static std::mutex sMutex;

    // 1. Define an RAII struct that triggers automatically when the program
    // unloads from memory
    struct TrayThreadGuard {
        ~TrayThreadGuard() {
            // Force break the GLib loop if it's active
            if (sMainLoop) {
                g_main_loop_quit(sMainLoop);
            }
            // Explicitly join the thread to safely park execution before main
            // unloads
            if (sWorkerThread.joinable()) {
                sWorkerThread.join();
            }
        }
    };

    // 2. Instantiate it as an inline static member so its lifetime matches the
    // global binary context
    inline static TrayThreadGuard sThreadGuard;
};
