#pragma once

#include <gio/gdesktopappinfo.h>
#include <gio/gio.h>

#include <cstdlib>
#include <filesystem>
#include <string>

#include "../../core_shell/Types.h"

namespace fs = std::filesystem;

class LinuxLauncher {
   public:
    // -------------------------------------------------------------------------
    // Open URL in default web browser
    // -------------------------------------------------------------------------

    static bool openUrl(const UrlLaunchOptions& ulopts) {
        if (ulopts.url.empty()) return false;

        GError* error = nullptr;
        gboolean success =
            g_app_info_launch_default_for_uri(ulopts.url.c_str(),
                                              nullptr,  // GAppLaunchContext
                                              &error);

        if (error) {
            g_error_free(error);
            error = nullptr;
        }

        // Fallback to xdg-open if GIO default launch fails
        if (!success) {
            std::string cmd = "xdg-open " + escapeShellArg(ulopts.url) +
                              " > /dev/null 2>&1 &";
            return (std::system(cmd.c_str()) == 0);
        }

        return success != 0;
    }

    // -------------------------------------------------------------------------
    // Open file or directory using default handler
    // -------------------------------------------------------------------------

    static bool openFile(const FileLaunchOptions& flopts) {
        if (flopts.filePath.empty() || !fs::exists(flopts.filePath)) {
            return false;
        }

        // Convert path to file URI (e.g., file:///path/to/file)
        std::string fileUri =
            "file://" + fs::absolute(flopts.filePath).string();

        GError* error = nullptr;
        gboolean success =
            g_app_info_launch_default_for_uri(fileUri.c_str(), nullptr, &error);

        if (error) {
            g_error_free(error);
            error = nullptr;
        }

        // Fallback to xdg-open
        if (!success) {
            std::string cmd = "xdg-open " +
                              escapeShellArg(flopts.filePath.string()) +
                              " > /dev/null 2>&1 &";
            return (std::system(cmd.c_str()) == 0);
        }

        return success != 0;
    }

    // -------------------------------------------------------------------------
    // Launch an application (via .desktop ID or binary command)
    // -------------------------------------------------------------------------

    static bool openApplication(const ApplicationLaunchOptions& alopts) {
        // 1. Try launching by Desktop Entry ID if provided
        if (!alopts.appId.empty()) {
            std::string desktopId = sanitizeDesktopId(alopts.appId);
            GDesktopAppInfo* appInfo =
                g_desktop_app_info_new(desktopId.c_str());

            if (appInfo) {
                GError* error = nullptr;
                gboolean success =
                    g_app_info_launch(G_APP_INFO(appInfo),
                                      nullptr,  // List of files to open
                                      nullptr,  // Launch context
                                      &error);

                if (error) {
                    g_error_free(error);
                }

                g_object_unref(appInfo);
                if (success) return true;
            }
        }

        // 2. Fall back to binary launch via command
        if (!alopts.command.empty()) {
            std::string cmd = escapeShellArg(alopts.command);
            for (const auto& arg : alopts.args) {
                cmd += " " + escapeShellArg(arg);
            }
            cmd += " &";  // Non-blocking execution

            return (std::system(cmd.c_str()) == 0);
        }

        return false;
    }

   private:
    static std::string sanitizeDesktopId(const std::string& input) {
        if (input.size() < 8 || input.substr(input.size() - 8) != ".desktop") {
            return input + ".desktop";
        }
        return input;
    }

    static std::string escapeShellArg(const std::string& arg) {
        std::string escaped = "'";
        for (char c : arg) {
            if (c == '\'') {
                escaped += "'\\''";
            } else {
                escaped += c;
            }
        }
        escaped += "'";
        return escaped;
    }
};
