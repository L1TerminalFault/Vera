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
        std::mutex readyMutex;
        std::condition_variable readyCv;

        // Spin up a completely isolated background worker thread
        sWorkerThread = std::thread([&]() {
            // 1. Force this thread to use its own isolated event context
            GMainContext* privateContext = g_main_context_new();
            g_main_context_push_thread_default(privateContext);

            GError* error = nullptr;
            sBus = g_bus_get_sync(G_BUS_TYPE_SESSION, nullptr, &error);
            if (!sBus) {
                if (error) g_error_free(error);
                g_main_context_pop_thread_default(privateContext);
                g_main_context_unref(privateContext);
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
                "    <property name='ToolTip' type='(ssss)' access='read'/>"
                "    <signal name='NewTitle'/>"
                "    <signal name='NewIcon'/>"
                "    <signal name='NewStatus'/>"
                "    <signal name='NewToolTip'/>"
                "  </interface>"
                "</node>";

            GDBusNodeInfo* nodeInfo =
                g_dbus_node_info_new_for_xml(introspectionXml, &error);
            if (error) {
                g_error_free(error);
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
                    if (std::string(propertyName) == "Category") {
                        return g_variant_new_string("ApplicationStatus");
                    }
                    if (std::string(propertyName) == "Id") {
                        return g_variant_new_string(sOptions.id.c_str());
                    }
                    if (std::string(propertyName) == "Title") {
                        return g_variant_new_string(sOptions.title.c_str());
                    }
                    if (std::string(propertyName) == "Status") {
                        return g_variant_new_string("Active");
                    }
                    if (std::string(propertyName) == "IconName") {
                        return g_variant_new_string(sOptions.iconName.c_str());
                    }
                    if (std::string(propertyName) == "ToolTip") {
                        GVariantBuilder arrayBuilder;
                        g_variant_builder_init(&arrayBuilder,
                                               G_VARIANT_TYPE("a(iiay)"));
                        GVariant* emptyIconData =
                            g_variant_builder_end(&arrayBuilder);

                        return g_variant_new(
                            "(s@a(iiay)ss)", sOptions.iconName.c_str(),
                            emptyIconData, sOptions.title.c_str(),
                            sOptions.tooltip.c_str());
                    }
                    return nullptr;
                },
                nullptr,
                {nullptr}};

            sRegistrationId = g_dbus_connection_register_object(
                sBus, sObjectPath.c_str(),
                nodeInfo->interfaces[0],  // Explicitly pass the interface index
                                          // object pointer
                &interfaceVtable, nullptr, nullptr, &error);

            g_dbus_node_info_unref(nodeInfo);

            if (error) {
                g_error_free(error);
                return;
            }

            // Register our background-hosted item with the central system
            // watcher
            GVariant* reply = g_dbus_connection_call_sync(
                sBus, "org.kde.StatusNotifierWatcher", "/StatusNotifierWatcher",
                "org.kde.StatusNotifierWatcher", "RegisterStatusNotifierItem",
                g_variant_new("(s)", sServiceName.c_str()), nullptr,
                G_DBUS_CALL_FLAGS_NONE, -1, nullptr, &error);

            if (error) {
                g_error_free(error);
                return;
            }
            if (reply) g_variant_unref(reply);

            // Instantiate main loop on our private context layout
            sMainLoop = g_main_loop_new(privateContext, FALSE);

            // 2. Safe notification handshake: Tell the main thread we are fully
            // up and looping
            {
                std::lock_guard<std::mutex> readyLock(readyMutex);
                threadReady = true;
                sIsCreated = true;
            }
            readyCv.notify_one();

            // Loop blocks here independently on the worker thread, serving
            // incoming queries instantly
            g_main_loop_run(sMainLoop);

            // Cleanup when loop quits
            g_main_loop_unref(sMainLoop);
            sMainLoop = nullptr;
            g_main_context_pop_thread_default(privateContext);
            g_main_context_unref(privateContext);
        });

        // 3. Main thread blocks safely via Condition Variable until the worker
        // thread is ready
        std::unique_lock<std::mutex> readyLock(readyMutex);
        readyCv.wait(readyLock, [&] { return threadReady; });

        return true;
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
