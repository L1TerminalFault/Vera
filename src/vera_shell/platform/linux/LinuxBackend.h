#include <canberra.h>  // libcanberra standard XDG sound library
#include <gio/gdesktopappinfo.h>
#include <gio/gio.h>

#include <condition_variable>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <thread>

#include "../../core_shell/Types.h"

class LinuxBackend : public IPlatformBackend {
   public:
    LinuxBackend() = default;
    ~LinuxBackend() = default;

    NotificationResult showNotification(const NotificationOptions&) override;
    bool closeNotification(std::uint32_t) override;

    DialogResult showDialog(const DialogOptions&) override;
    nostd::Path openFileDialog(const FileDialogOptions&) override;
    nostd::Path saveFileDialog(const SaveFileDialogOptions&) override;

    bool createTray(const TrayOptions&) override;
    bool updateTray(const TrayUpdate&) override;
    bool removeTray() override;

    bool setBadge(const BadgeOptions&) override;
    bool clearBadge(const nostd::String&) override;

    bool openUrl(const UrlLaunchOptions&) override;
    bool openFile(const FileLaunchOptions&) override;
    bool openApplication(const ApplicationLaunchOptions&) override;

    bool registerAssociation(const FileAssociation&) override;
    bool unregisterAssociation(const nostd::String&) override;
    bool isAssociationRegistered(const nostd::String&) override;

    bool playSound(SystemSound) override;

    bool preventSleep(const SleepRequest&) override;
    bool allowSleep(SleepTarget) override;

    bool requestAttention(AttentionType, const nostd::String&) override;

    bool setProgress(const ProgressOptions&) override;
    bool clearProgress(const nostd::String&) override;

    bool setEnvironmentVariable(const nostd::String& name,
                                const nostd::String& value) override;
    nostd::Optional<nostd::String> getEnvironmentVariable(
        const nostd::String& name) override;
    bool unsetEnvironmentVariable(const nostd::String& name) override;

    bool addPathToEnvironment(const nostd::Path& pathToAdd,
                              bool persistent) override;
    bool removePathFromEnvironment(const nostd::Path& pathToRemove,
                                   bool persistent) override;
};

namespace fs = std::filesystem;

class LinuxAssociation {
   public:
    static bool registerAssociation(const FileAssociation& fassoc) {
        if (fassoc.extension.empty() || fassoc.mimeType.empty() ||
            fassoc.command.empty()) {
            return false;
        }

        nostd::String appName = sanitizeName(fassoc.mimeType);
        nostd::String desktopFileName = "app-assoc-" + appName + ".desktop";
        nostd::Path desktopFilePath = getLocalAppsPath() / desktopFileName;

        // 1. Ensure ~/.local/share/applications/ directory exists
        std::error_code ec;
        fs::create_directories(getLocalAppsPath().c_str(), ec);
        if (ec) return false;

        // 2. Ensure launch command has XDG file placeholder (%f, %F, %u, or %U)
        nostd::String formattedCmd = fassoc.command;
        if (formattedCmd.find("%f") == nostd::String::npos &&
            formattedCmd.find("%F") == nostd::String::npos &&
            formattedCmd.find("%u") == nostd::String::npos &&
            formattedCmd.find("%U") == nostd::String::npos) {
            formattedCmd += " %f";
        }

        // 3. Create the .desktop file required by Freedesktop standards
        std::ofstream desktopFile(desktopFilePath.c_str());
        if (!desktopFile.is_open()) return false;

        desktopFile << "[Desktop Entry]\n"
                    << "Type=Application\n"
                    << "Name="
                    << (fassoc.description.empty() ? appName.c_str()
                                                   : fassoc.description.c_str())
                    << "\n"
                    << "Exec=" << formattedCmd.c_str() << "\n"
                    << "MimeType=" << fassoc.mimeType.c_str() << ";\n"
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
            G_APP_INFO(appInfo), fassoc.mimeType.c_str(), &error);

        if (error) {
            g_error_free(error);
        }

        g_object_unref(appInfo);
        return success != 0;
    }

    static bool unregisterAssociation(const nostd::String& mimeType) {
        if (mimeType.empty()) return false;

        // Query who is current default handler
        GAppInfo* defaultApp =
            g_app_info_get_default_for_type(mimeType.c_str(), FALSE);
        if (!defaultApp) return true;  // Already unassociated

        nostd::String appName = sanitizeName(mimeType);
        nostd::String desktopFileName = "app-assoc-" + appName + ".desktop";
        nostd::Path desktopFilePath = getLocalAppsPath() / desktopFileName;

        // Remove the local .desktop file
        std::error_code ec;
        if (fs::exists(desktopFilePath.c_str())) {
            fs::remove(desktopFilePath.c_str(), ec);
        }

        g_object_unref(defaultApp);
        return !ec;
    }

    static bool isAssociationRegistered(const nostd::String& mimeType) {
        if (mimeType.empty()) return false;

        // Native query to retrieve default handler via GIO
        GAppInfo* appInfo =
            g_app_info_get_default_for_type(mimeType.c_str(), FALSE);
        if (!appInfo) {
            return false;
        }

        // Verify if a valid desktop entry is returned
        const char* appId = g_app_info_get_id(appInfo);
        bool registered =
            (appId != nullptr && nostd::String(appId).length() > 0);

        g_object_unref(appInfo);
        return registered;
    }

   private:
    static nostd::Path getLocalAppsPath() {
        const char* dataHome = std::getenv("XDG_DATA_HOME");
        if (dataHome && *dataHome) {
            return nostd::Path(dataHome) / "applications";
        }

        const char* home = std::getenv("HOME");
        if (home && *home) {
            return nostd::Path(home) / ".local" / "share" / "applications";
        }

        return nostd::Path("/tmp");
    }

    static nostd::String sanitizeName(const nostd::String& input) {
        nostd::String output = input;
        for (char& c : output) {
            if (c == '/' || c == ' ') c = '-';
        }
        return output;
    }
};

class LinuxBadge {
   public:
    static bool setBadge(const BadgeOptions& bopts) {
        if (bopts.appId.empty()) {
            return false;
        }

        // Must match your app's actual .desktop filename
        nostd::String sanitizedId = sanitizeAppId(bopts.appId);
        nostd::String appUri = "application://" + sanitizedId;

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

    static bool clearBadge(const nostd::String& appId) {
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

    static nostd::String sanitizeAppId(const nostd::String& input) {
        if (input.size() < 8 || input.substr(input.size() - 8) != ".desktop") {
            return input + ".desktop";
        }
        return input;
    }
};

class LinuxDialog {
   public:
    // -------------------------------------------------------------------------
    // Message Dialogs
    // -------------------------------------------------------------------------

    static DialogResult showDialog(const DialogOptions& dopts) {
        DialogResult result;
        Tool tool = detectAvailableTool();

        if (tool == Tool::Zenity) {
            nostd::String cmd =
                "zenity --question --title=" + escapeShellArg(dopts.title) +
                " --text=" + escapeShellArg(dopts.message);

            switch (dopts.icon) {
                case DialogIcon::Info:
                    cmd += " --icon-name=dialog-information";
                    break;
                case DialogIcon::Warning:
                    cmd += " --icon-name=dialog-warning";
                    break;
                case DialogIcon::Error:
                    cmd += " --icon-name=dialog-error";
                    break;
                case DialogIcon::Question:
                    cmd += " --icon-name=dialog-question";
                    break;
                default:
                    break;
            }

            // Custom buttons
            for (const auto& btn : dopts.buttons) {
                cmd += " --extra-button=" + escapeShellArg(buttonToString(btn));
            }

            int exitCode = 0;
            nostd::String output =
                executeCommandWithOutputAndStatus(cmd, exitCode);

            // Strip trailing newline / whitespace returned by zenity stdout
            while (!output.empty() &&
                   (output.back() == '\n' || output.back() == '\r')) {
                output.pop_back();
            }

            if (!output.empty()) {
                // User clicked an extra button or typed input
                result.button = stringToButton(output);
                result.customButtonText = output;
            } else if (exitCode == 0) {
                // Clicked OK / Yes (Default Zenity success exit code)
                result.button = DialogButton::Yes;
            } else {
                // Clicked Cancel / No or closed window (Exit code 1)
                result.button = DialogButton::Cancel;
            }
        } else if (tool == Tool::KDialog) {
            nostd::String cmd = "kdialog";
            switch (dopts.icon) {
                case DialogIcon::Warning:
                    cmd += " --warningyesno";
                    break;
                case DialogIcon::Error:
                    cmd += " --error";
                    break;
                case DialogIcon::Question:
                    cmd += " --yesno";
                    break;
                default:
                    cmd += " --msgbox";
                    break;
            }

            cmd += " " + escapeShellArg(dopts.message) + " --title " +
                   escapeShellArg(dopts.title);
            int status = executeCommand(cmd);
            result.button =
                (status == 0) ? DialogButton::Ok : DialogButton::Cancel;
        } else {
            // TTY / Console Fallback
            printf("\n========================================\n");
            printf("[%s]\n", dopts.title.c_str());
            printf("%s\n", dopts.message.c_str());
            printf("========================================\n");

            if (dopts.buttons.empty()) {
                printf("Press [Enter] to continue...");
                fflush(stdout);
                getchar();
                result.button = DialogButton::Ok;
            } else {
                printf("Options: ");
                for (size_t i = 0; i < dopts.buttons.size(); ++i) {
                    printf("(%zu) %s ", i + 1,
                           buttonToString(dopts.buttons[i]).c_str());
                }
                printf("\nChoice: ");
                fflush(stdout);

                int choice = 1;
                char inputBuf[32];
                if (fgets(inputBuf, sizeof(inputBuf), stdin) != nullptr) {
                    if (sscanf(inputBuf, "%d", &choice) != 1) {
                        choice = 1;
                    }
                }

                if (choice >= 1 &&
                    choice <= static_cast<int>(dopts.buttons.size())) {
                    result.button = dopts.buttons[choice - 1];
                } else {
                    result.button = dopts.buttons[0];
                }
            }
        }

        return result;
    }

    // -------------------------------------------------------------------------
    // Open File Dialog
    // -------------------------------------------------------------------------

    static nostd::Path openFileDialog(const FileDialogOptions& fdopts) {
        Tool tool = detectAvailableTool();

        if (tool == Tool::Zenity) {
            int exitCode = 0;
            nostd::String cmd = "zenity --file-selection --title=" +
                                escapeShellArg(fdopts.title);
            if (!fdopts.defaultPath.empty()) {
                cmd += " --filename=" +
                       escapeShellArg(fdopts.defaultPath.string());
            }
            nostd::String output =
                executeCommandWithOutputAndStatus(cmd, exitCode);

            // Return selected path or empty path if cancelled (exit code 1)
            if (exitCode == 0 && !output.empty()) return nostd::Path(output);
            if (exitCode == 1) return "";  // User explicitly cancelled
        }

        if (tool == Tool::KDialog) {
            int exitCode = 0;
            nostd::String startDir =
                fdopts.defaultPath.empty() ? "." : fdopts.defaultPath.string();
            nostd::String cmd = "kdialog --getopenfilename " +
                                escapeShellArg(startDir) + " --title " +
                                escapeShellArg(fdopts.title);
            nostd::String output =
                executeCommandWithOutputAndStatus(cmd, exitCode);

            if (exitCode == 0 && !output.empty()) return nostd::Path(output);
            if (exitCode == 1) return "";  // User explicitly cancelled
        }

        if (tool == Tool::XdgPortal) {
            bool userCancelled = false;
            nostd::Path resultPath = openFileViaPortal(fdopts.title, false);

            if (!resultPath.empty()) return resultPath;
            if (userCancelled) {
                return "";
            }
        }

        // Only hit this if NO dialog tool could be run at all
        printf("\n[%s]\nEnter file path to open (or press Enter to cancel): ",
               fdopts.title.c_str());
        fflush(stdout);

        char buf[1024];
        nostd::String consoleInput;

        if (fgets(buf, sizeof(buf), stdin) != nullptr) {
            // Skip leading whitespace (std::ws equivalent)
            char* start = buf;
            while (*start == ' ' || *start == '\t' || *start == '\r' ||
                   *start == '\n') {
                start++;
            }

            // Strip trailing newline / carriage return
            size_t len = strlen(start);
            while (len > 0 &&
                   (start[len - 1] == '\n' || start[len - 1] == '\r')) {
                start[--len] = '\0';
            }

            consoleInput = nostd::String(start);
        }
        return nostd::Path(consoleInput);
    }

    // -------------------------------------------------------------------------
    // Save File Dialog
    // -------------------------------------------------------------------------

    static nostd::Path saveFileDialog(const SaveFileDialogOptions& sfdopts) {
        Tool tool = detectAvailableTool();

        if (tool == Tool::Zenity) {
            nostd::Path initialPath = sfdopts.defaultPath / sfdopts.defaultName;
            nostd::String cmd =
                "zenity --file-selection --save --confirm-overwrite --title=" +
                escapeShellArg(sfdopts.title);
            if (!initialPath.empty()) {
                cmd += " --filename=" + escapeShellArg(initialPath.string());
            }
            nostd::String output = executeCommandWithOutput(cmd);
            if (!output.empty()) return nostd::Path(output);
        }

        if (tool == Tool::KDialog) {
            nostd::Path initialPath = sfdopts.defaultPath / sfdopts.defaultName;
            nostd::String startDir =
                initialPath.empty() ? "." : initialPath.string();
            nostd::String cmd = "kdialog --getsavefilename " +
                                escapeShellArg(startDir) + " --title " +
                                escapeShellArg(sfdopts.title);
            nostd::String output = executeCommandWithOutput(cmd);
            if (!output.empty()) return nostd::Path(output);
        }

        if (tool == Tool::XdgPortal) {
            nostd::Path resultPath = openFileViaPortal(sfdopts.title, true);
            if (!resultPath.empty()) return resultPath;
        }

        printf("\n[%s]\nEnter destination path to save: ",
               sfdopts.title.c_str());
        fflush(stdout);

        char buf[1024];
        nostd::String consoleInput;

        if (fgets(buf, sizeof(buf), stdin) != nullptr) {
            // Skip leading whitespace (std::ws equivalent)
            char* start = buf;
            while (*start == ' ' || *start == '\t' || *start == '\r' ||
                   *start == '\n') {
                start++;
            }

            // Strip trailing newline / carriage return
            size_t len = strlen(start);
            while (len > 0 &&
                   (start[len - 1] == '\n' || start[len - 1] == '\r')) {
                start[--len] = '\0';
            }

            consoleInput = nostd::String(start);
        }
        return nostd::Path(consoleInput);
    }

   private:
    enum class Tool { Zenity, KDialog, XdgPortal, None };

    static Tool detectAvailableTool() {
        if (executeCommand("which zenity > /dev/null 2>&1") == 0) {
            return Tool::Zenity;
        }
        if (executeCommand("which kdialog > /dev/null 2>&1") == 0) {
            return Tool::KDialog;
        }
        return Tool::XdgPortal;
    }

    static nostd::Path openFileViaPortal(const nostd::String& title,
                                         bool isSave) {
        GError* error = nullptr;
        GDBusConnection* bus =
            g_bus_get_sync(G_BUS_TYPE_SESSION, nullptr, &error);
        if (!bus) {
            if (error) g_error_free(error);
            return "";
        }

        GVariantBuilder optBuilder;
        g_variant_builder_init(&optBuilder, G_VARIANT_TYPE("a{sv}"));
        g_variant_builder_add(&optBuilder, "{sv}", "handle_token",
                              g_variant_new_string("vera_file_token"));

        GVariant* options = g_variant_builder_end(&optBuilder);

        const char* method = isSave ? "SaveFile" : "OpenFile";

        GVariant* reply = g_dbus_connection_call_sync(
            bus, "org.freedesktop.portal.Desktop",
            "/org/freedesktop/portal/desktop",
            "org.freedesktop.portal.FileChooser", method,
            g_variant_new("(ss@a{sv})", "", title.c_str(), options),
            G_VARIANT_TYPE("(o)"), G_DBUS_CALL_FLAGS_NONE, -1, nullptr, &error);

        if (!reply) {
            if (error) g_error_free(error);
            g_object_unref(bus);
            return "";
        }

        const char* requestPath = nullptr;
        g_variant_get(reply, "(&o)", &requestPath);

        nostd::Path chosenPath = "";
        GMainLoop* mainLoop = g_main_loop_new(nullptr, FALSE);

        struct SignalContext {
            nostd::Path* path;
            GMainLoop* loop;
            bool cancelled;
        } context = {&chosenPath, mainLoop, false};

        guint signalId = g_dbus_connection_signal_subscribe(
            bus, "org.freedesktop.portal.Desktop",
            "org.freedesktop.portal.Request", "Response", requestPath, nullptr,
            G_DBUS_SIGNAL_FLAGS_NONE,
            [](GDBusConnection*, const char*, const char*, const char*,
               const char*, GVariant* parameters, gpointer user_data) {
                auto* ctx = static_cast<SignalContext*>(user_data);
                std::uint32_t responseCode =
                    1;  // 0 = Success, 1 = Cancelled, 2 = Dismissed
                GVariant* resultsDict = nullptr;

                g_variant_get(parameters, "(u@a{sv})", &responseCode,
                              &resultsDict);

                if (responseCode != 0) {
                    (ctx->cancelled) =
                        true;  // Mark that user explicitly cancelled
                } else if (resultsDict) {
                    // Extract file path...
                }

                if (resultsDict) g_variant_unref(resultsDict);
                g_main_loop_quit(ctx->loop);
            },
            &context, nullptr);

        g_main_loop_run(mainLoop);

        g_dbus_connection_signal_unsubscribe(bus, signalId);
        g_main_loop_unref(mainLoop);
        g_variant_unref(reply);
        g_object_unref(bus);

        return chosenPath;
    }

    static int executeCommand(const nostd::String& cmd) {
        return std::system(cmd.c_str());
    }

    static nostd::String executeCommandWithOutput(const nostd::String& cmd) {
        int dummyStatus = 0;
        return executeCommandWithOutputAndStatus(cmd, dummyStatus);
    }

    static nostd::String executeCommandWithOutputAndStatus(
        const nostd::String& cmd, int& outExitCode) {
        std::array<char, 256> buffer;
        nostd::String result;

        FILE* pipe = popen(cmd.c_str(), "r");
        if (!pipe) {
            outExitCode = -1;
            return "";
        }

        while (fgets(buffer.data(), static_cast<int>(buffer.size()), pipe) !=
               nullptr) {
            result += buffer.data();
        }

        int status = pclose(pipe);
        if (WIFEXITED(status)) {
            outExitCode = WEXITSTATUS(status);
        } else {
            outExitCode = -1;
        }

        // Safe String Trimming
        if (!result.empty()) {
            size_t endpos = result.find_last_not_of(" \n\r\t");
            if (endpos != nostd::String::npos) {
                result.erase(endpos + 1);
            } else {
                result.clear();
            }
        }

        return result;
    }

    static nostd::String escapeShellArg(const nostd::String& arg) {
        nostd::String escaped = "'";
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

    static nostd::String buttonToString(DialogButton btn) {
        switch (btn) {
            case DialogButton::Ok:
                return "OK";
            case DialogButton::Cancel:
                return "Cancel";
            case DialogButton::Yes:
                return "Yes";
            case DialogButton::No:
                return "No";
            default:
                return "Custom";
        }
    }

    static DialogButton stringToButton(const nostd::String& str) {
        if (str == "OK") return DialogButton::Ok;
        if (str == "Yes") return DialogButton::Yes;
        if (str == "Cancel") return DialogButton::Cancel;
        if (str == "No") return DialogButton::Cancel;
        return DialogButton::Custom;
    }
};

class LinuxEnvironment {
   public:
    // -------------------------------------------------------------------------
    // Basic Environment Variables (Process-Level)
    // -------------------------------------------------------------------------

    static bool setEnvironmentVariable(const nostd::String& name,
                                       const nostd::String& value) {
        if (name.empty()) return false;
        // setenv(name, value, overwrite = 1) -> 0 on success
        return setenv(name.c_str(), value.c_str(), 1) == 0;
    }

    static nostd::Optional<nostd::String> getEnvironmentVariable(
        const nostd::String& name) {
        if (name.empty()) {
            return nostd::Optional<nostd::String>();
        }

        const char* val = std::getenv(name.c_str());
        if (val != nullptr) {
            return nostd::Optional<nostd::String>(nostd::String(val));
        }

        return nostd::Optional<nostd::String>();
    }

    static bool unsetEnvironmentVariable(const nostd::String& name) {
        if (name.empty()) return false;
        // unsetenv(name) -> 0 on success
        return unsetenv(name.c_str()) == 0;
    }

    // -------------------------------------------------------------------------
    // Path Operations
    // -------------------------------------------------------------------------

    static bool addPathToEnvironment(const nostd::Path& pathToAdd,
                                     bool persistent) {
        if (pathToAdd.empty()) return false;

        nostd::String targetPath = pathToAdd.string();

        // 1. Process-level update
        nostd::Optional<nostd::String> currentPathOpt =
            getEnvironmentVariable("PATH");
        nostd::String currentPath =
            currentPathOpt ? currentPathOpt.value() : "";

        if (!containsPathEntry(currentPath, targetPath)) {
            nostd::String newPath = currentPath.empty()
                                        ? targetPath
                                        : (targetPath + ":" + currentPath);
            if (!setEnvironmentVariable("PATH", newPath)) {
                return false;
            }
        }

        if (!persistent) return true;

        // 2. Persistent update (Shell configs: ~/.bashrc, ~/.zshrc, ~/.profile)
        const char* homeDir = std::getenv("HOME");
        if (!homeDir) return false;

        nostd::Vector<nostd::Path> targetConfigs;
        targetConfigs.push_back(nostd::Path(homeDir) / ".bashrc");
        targetConfigs.push_back(nostd::Path(homeDir) / ".zshrc");
        targetConfigs.push_back(nostd::Path(homeDir) / ".profile");

        nostd::String exportStatement =
            "\n# Added by VeraShell\nexport PATH=\"" + targetPath +
            ":$PATH\"\n";

        bool persistentSuccess = false;

        for (const auto& configPath : targetConfigs) {
            if (fs::exists(configPath.c_str())) {
                if (!fileContainsString(configPath, targetPath)) {
                    std::ofstream outFile(configPath.c_str(), std::ios::app);
                    if (outFile.is_open()) {
                        outFile << exportStatement;
                        persistentSuccess = true;
                    }
                } else {
                    persistentSuccess = true;  // Already registered
                }
            }
        }

        return persistentSuccess;
    }

    static bool removePathFromEnvironment(const nostd::Path& pathToRemove,
                                          bool persistent) {
        if (pathToRemove.empty()) return false;

        nostd::String targetPath = pathToRemove.string();

        // 1. Process-level removal
        nostd::Optional<nostd::String> currentPathOpt =
            getEnvironmentVariable("PATH");
        if (currentPathOpt) {
            nostd::Vector<nostd::String> entries =
                splitPath(currentPathOpt.value());
            nostd::String rebuiltPath;

            for (const auto& entry : entries) {
                if (nostd::Path(entry).string() != targetPath) {
                    if (!rebuiltPath.empty()) rebuiltPath += ":";
                    rebuiltPath += entry;
                }
            }

            setEnvironmentVariable("PATH", rebuiltPath);
        }

        if (!persistent) return true;

        // 2. Persistent removal from shell config files
        const char* homeDir = std::getenv("HOME");
        if (!homeDir) return false;

        nostd::Vector<nostd::Path> targetConfigs;
        targetConfigs.push_back(nostd::Path(homeDir) / ".bashrc");
        targetConfigs.push_back(nostd::Path(homeDir) / ".zshrc");
        targetConfigs.push_back(nostd::Path(homeDir) / ".profile");

        for (const auto& configPath : targetConfigs) {
            if (fs::exists(configPath.c_str())) {
                removeMatchingLinesFromFile(configPath, targetPath);
            }
        }

        return true;
    }

   private:
    // Helper to split a colon-separated PATH string into individual components
    static nostd::Vector<nostd::String> splitPath(
        const nostd::String& pathStr) {
        nostd::Vector<nostd::String> tokens;
        std::stringstream ss(pathStr.c_str());
        nostd::String token;
        while (getline(ss, token, ':')) {
            if (!token.empty()) {
                tokens.push_back(token);
            }
        }
        return tokens;
    }

    // Checks if a path entry exists inside the current process PATH variable
    static bool containsPathEntry(const nostd::String& pathEnv,
                                  const nostd::String& targetPath) {
        nostd::Vector<nostd::String> entries = splitPath(pathEnv);
        for (const auto& entry : entries) {
            if (nostd::Path(entry).string() == targetPath) {
                return true;
            }
        }
        return false;
    }

    // Helper to check if a file contains a specific export entry
    static bool fileContainsString(const nostd::Path& filePath,
                                   const nostd::String& needle) {
        std::ifstream file(filePath.c_str());
        if (!file.is_open()) return false;

        nostd::String line;
        while (getline(file, line)) {
            if (line.find(needle.c_str()) != nostd::String::npos) {
                return true;
            }
        }
        return false;
    }

    // Helper to remove path entries and associated comments safely
    static void removeMatchingLinesFromFile(const nostd::Path& filePath,
                                            const nostd::String& targetPath) {
        std::ifstream inFile(filePath.c_str());
        if (!inFile.is_open()) return;

        nostd::Vector<nostd::String> lines;
        nostd::String line;
        bool modified = false;

        while (getline(inFile, line)) {
            bool isTargetLine =
                (line.find("export PATH=") != nostd::String::npos ||
                 line.find("PATH=") != nostd::String::npos) &&
                (line.find(targetPath.c_str()) != nostd::String::npos);

            if (isTargetLine) {
                modified = true;
                if (!lines.empty() &&
                    lines.back().find("# Added by VeraShell") !=
                        nostd::String::npos) {
                    lines.pop_back();
                }
                continue;
            }
            lines.push_back(line);
        }
        inFile.close();

        if (modified) {
            std::ofstream outFile(filePath.c_str(), std::ios::trunc);
            if (outFile.is_open()) {
                for (const auto& l : lines) {
                    outFile << l << "\n";
                }
            }
        }
    }
};

class LinuxIntegration {
   public:
    // -------------------------------------------------------------------------
    // Set / Clear Progress Bar on Dock Icon (via D-Bus)
    // -------------------------------------------------------------------------

    static bool setProgress(const ProgressOptions& popts) {
        if (popts.appId.empty()) {
            return false;
        }

        nostd::String appUri = "application://" + sanitizeAppId(popts.appId);

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

    static bool clearProgress(const nostd::String& appId) {
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
                                 const nostd::String& appId) {
        if (appId.empty()) return false;

        nostd::String appUri = "application://" + sanitizeAppId(appId);

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

    static nostd::String sanitizeAppId(const nostd::String& input) {
        if (input.size() < 8 || input.substr(input.size() - 8) != ".desktop") {
            return input + ".desktop";
        }
        return input;
    }
};

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
            nostd::String cmd = "xdg-open " + escapeShellArg(ulopts.url) +
                                " > /dev/null 2>&1 &";
            return (std::system(cmd.c_str()) == 0);
        }

        return success != 0;
    }

    // -------------------------------------------------------------------------
    // Open file or directory using default handler
    // -------------------------------------------------------------------------

    static bool openFile(const FileLaunchOptions& flopts) {
        if (flopts.filePath.empty() || !fs::exists(flopts.filePath.c_str())) {
            return false;
        }

        // Convert path to file URI (e.g., file:///path/to/file)
        nostd::String fileUri =
            "file://" +
            nostd::String(fs::absolute(flopts.filePath.c_str()).c_str());

        GError* error = nullptr;
        gboolean success =
            g_app_info_launch_default_for_uri(fileUri.c_str(), nullptr, &error);

        if (error) {
            g_error_free(error);
            error = nullptr;
        }

        // Fallback to xdg-open
        if (!success) {
            nostd::String cmd = "xdg-open " +
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
            nostd::String desktopId = sanitizeDesktopId(alopts.appId);
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
            nostd::String cmd = escapeShellArg(alopts.command);
            for (const auto& arg : alopts.args) {
                cmd += " " + escapeShellArg(arg);
            }
            cmd += " &";  // Non-blocking execution

            return (std::system(cmd.c_str()) == 0);
        }

        return false;
    }

   private:
    static nostd::String sanitizeDesktopId(const nostd::String& input) {
        if (input.size() < 8 || input.substr(input.size() - 8) != ".desktop") {
            return input + ".desktop";
        }
        return input;
    }

    static nostd::String escapeShellArg(const nostd::String& arg) {
        nostd::String escaped = "'";
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

        GVariant* options = g_variant_builder_end(&optBuilder);

        GVariant* reply = g_dbus_connection_call_sync(
            bus, "org.freedesktop.portal.Desktop",
            "/org/freedesktop/portal/desktop", "org.freedesktop.portal.Inhibit",
            "Inhibit", g_variant_new("(su@a{sv})", "", flags, options),
            G_VARIANT_TYPE("(o)"), G_DBUS_CALL_FLAGS_NONE, -1, nullptr, &error);

        if (reply) {
            g_variant_unref(reply);
            g_object_unref(bus);
            setCookie(slreq.target, 1);  // Mark active lock
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
            setCookie(slreq.target, cookie);
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
        std::uint32_t cookie = removeCookie(sltgt);
        if (cookie == 0) {
            return false;  // No active sleep inhibition found for target
        }

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
        return success;
    }

   private:
    struct CookieEntry {
        SleepTarget target;
        std::uint32_t cookie;
        bool active;
    };

    static constexpr std::size_t MAX_COOKIES = 8;

    static void setCookie(SleepTarget target, std::uint32_t cookie) {
        static CookieEntry entries[MAX_COOKIES]{};

        // 1. Update existing entry if present
        for (std::size_t i = 0; i < MAX_COOKIES; ++i) {
            if (entries[i].active && entries[i].target == target) {
                entries[i].cookie = cookie;
                return;
            }
        }

        // 2. Store in first available slot
        for (std::size_t i = 0; i < MAX_COOKIES; ++i) {
            if (!entries[i].active) {
                entries[i].target = target;
                entries[i].cookie = cookie;
                entries[i].active = true;
                return;
            }
        }
    }

    static std::uint32_t removeCookie(SleepTarget target) {
        static CookieEntry entries[MAX_COOKIES]{};

        for (std::size_t i = 0; i < MAX_COOKIES; ++i) {
            if (entries[i].active && entries[i].target == target) {
                entries[i].active = false;
                return entries[i].cookie;
            }
        }
        return 0;
    }

    static std::uint32_t translateTargetToFlags(SleepTarget target) {
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

class LinuxSound {
   public:
    static bool playSound(SystemSound snd) {
        const char* soundId = translateSoundToId(snd);
        if (!soundId) {
            return false;
        }

        ca_context* ctx = nullptr;
        int res = ca_context_create(&ctx);
        if (res != CA_SUCCESS || !ctx) {
            return false;
        }

        // Play sound event asynchronously using active Freedesktop sound theme
        res = ca_context_play(ctx,
                              0,  // Internal sound slot ID
                              CA_PROP_EVENT_ID,
                              soundId,  // Freedesktop sound event ID string
                              nullptr);

        ca_context_destroy(ctx);
        return (res == CA_SUCCESS);
    }

   private:
    static const char* translateSoundToId(SystemSound snd) {
        // Maps SystemSound enum values to official XDG Sound Theme
        // Specification identifiers
        switch (snd) {
            case SystemSound::Notification:
                return "message-new-instant";  // Standard notification chime
            case SystemSound::Warning:
                return "dialog-warning";  // System warning sound
            case SystemSound::Error:
                return "dialog-error";  // System error sound
            case SystemSound::Question:
                return "dialog-question";  // Prompt / question sound
            case SystemSound::Success:
                return "complete";  // Task completion / success sound
            default:
                return nullptr;
        }
    }
};

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

            sServiceName = "org.kde.StatusNotifierItem-" +
                           nostd::to_string(getpid()) + "-1";
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
                "    <property name='ToolTip' type='(sa(iiay)ss)' "
                "access='read'/>"
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
                    nostd::String prop(propertyName);
                    if (prop == "Category") {
                        return g_variant_new_string("ApplicationStatus");
                    }
                    if (prop == "Id") {
                        return g_variant_new_string(sOptions.id.c_str());
                    }
                    if (prop == "Title") {
                        return g_variant_new_string(sOptions.title.c_str());
                    }
                    if (prop == "Status") return g_variant_new_string("Active");
                    if (prop == "IconName") {
                        return g_variant_new_string(sOptions.iconName.c_str());
                    }
                    if (prop == "ToolTip") {
                        // Generates a valid (sa(iiay)ss) structure with an
                        // empty image byte array
                        return g_variant_new(
                            "(s@a(iiay)ss)",
                            sOptions.iconName.c_str(),  // Icon Name
                            g_variant_new_array(G_VARIANT_TYPE("(iiay)"),
                                                nullptr, 0),  // Empty Icon Data
                            sOptions.title.c_str(),           // Tooltip Title
                            sOptions.tooltip.c_str()  // Tooltip Description
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
    inline static nostd::String sServiceName;
    inline static nostd::String sObjectPath;
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
