#pragma once

#include <gio/gdesktopappinfo.h>
#include <gio/gio.h>

#include <filesystem>
#include <fstream>
#include <string>

#include "../../core_shell/Types.h"

namespace fs = std::filesystem;

class LinuxAssociation {
   public:
    static bool registerAssociation(const FileAssociation& fassoc) {
        if (fassoc.extension.empty() || fassoc.mimeType.empty() ||
            fassoc.command.empty()) {
            return false;
        }

        std::string appName = sanitizeName(fassoc.mimeType);
        std::string desktopFileName = "app-assoc-" + appName + ".desktop";
        fs::path desktopFilePath = getLocalAppsPath() / desktopFileName;

        // 1. Ensure ~/.local/share/applications/ directory exists
        std::error_code ec;
        fs::create_directories(getLocalAppsPath(), ec);
        if (ec) return false;

        // 2. Ensure launch command has XDG file placeholder (%f)
        std::string formattedCmd = fassoc.command;
        if (formattedCmd.find("%f") == std::string::npos &&
            formattedCmd.find("%F") == std::string::npos &&
            formattedCmd.find("%u") == std::string::npos &&
            formattedCmd.find("%U") == std::string::npos) {
            formattedCmd += " %f";
        }

        // 3. Create the .desktop file required by Freedesktop standards
        std::ofstream desktopFile(desktopFilePath);
        if (!desktopFile.is_open()) return false;

        desktopFile << "[Desktop Entry]\n"
                    << "Type=Application\n"
                    << "Name="
                    << (fassoc.description.empty() ? appName
                                                   : fassoc.description)
                    << "\n"
                    << "Exec=" << formattedCmd << "\n"
                    << "MimeType=" << fassoc.mimeType << ";\n"
                    << "NoDisplay=true\n";
        desktopFile.close();

        // 4. Use GIO C-API to load the desktop file and set default application
        GDesktopAppInfo* appInfo =
            g_desktop_app_info_new(desktopFileName.c_str());
        if (!appInfo) {
            return false;
        }

        GError* error = nullptr;
        gboolean success = g_app_info_set_as_default_for_type(
            G_APP_INFO(appInfo), fassoc.mimeType.c_str(),
            // nullptr,  // GAppLaunchContext
            &error);

        if (error) {
            g_error_free(error);
        }

        g_object_unref(appInfo);
        return success != 0;
    }

    static bool unregisterAssociation(const std::string& mimeType) {
        if (mimeType.empty()) return false;

        // Query who is current default handler
        GAppInfo* defaultApp =
            g_app_info_get_default_for_type(mimeType.c_str(), FALSE);
        if (!defaultApp) return true;  // Already unassociated

        std::string appName = sanitizeName(mimeType);
        std::string desktopFileName = "app-assoc-" + appName + ".desktop";
        fs::path desktopFilePath = getLocalAppsPath() / desktopFileName;

        // Remove the local .desktop file
        std::error_code ec;
        if (fs::exists(desktopFilePath)) {
            fs::remove(desktopFilePath, ec);
        }

        g_object_unref(defaultApp);
        return !ec;
    }

    static bool isAssociationRegistered(const std::string& mimeType) {
        if (mimeType.empty()) return false;

        // Native query to retrieve default handler via GIO
        GAppInfo* appInfo =
            g_app_info_get_default_for_type(mimeType.c_str(), FALSE);
        if (!appInfo) {
            return false;
        }

        // Verify if a valid desktop entry is returned
        const char* appId = g_app_info_get_id(appInfo);
        bool registered = (appId != nullptr && std::string(appId).length() > 0);

        g_object_unref(appInfo);
        return registered;
    }

   private:
    static fs::path getLocalAppsPath() {
        const char* dataHome = std::getenv("XDG_DATA_HOME");
        if (dataHome && *dataHome) {
            return fs::path(dataHome) / "applications";
        }

        const char* home = std::getenv("HOME");
        if (home && *home) {
            return fs::path(home) / ".local" / "share" / "applications";
        }

        return fs::path("/tmp");
    }

    static std::string sanitizeName(const std::string& input) {
        std::string output = input;
        for (char& c : output) {
            if (c == '/' || c == ' ') c = '-';
        }
        return output;
    }
};
