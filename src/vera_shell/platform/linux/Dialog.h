#pragma once

#include <gio/gio.h>  // Native D-Bus for XDG Desktop Portals

#include <array>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

#include "../../core_shell/Types.h"

namespace fs = std::filesystem;

class LinuxDialog {
   public:
    // -------------------------------------------------------------------------
    // Message Dialogs
    // -------------------------------------------------------------------------

    static DialogResult showDialog(const DialogOptions& dopts) {
        DialogResult result;
        Tool tool = detectAvailableTool();

        if (tool == Tool::Zenity) {
            std::string cmd =
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
            std::string output =
                executeCommandWithOutputAndStatus(cmd, exitCode);

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
            std::string cmd = "kdialog";
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
            std::cout << "\n========================================\n";
            std::cout << "[" << dopts.title << "]\n";
            std::cout << dopts.message << "\n";
            std::cout << "========================================\n";

            if (dopts.buttons.empty()) {
                std::cout << "Press [Enter] to continue...";
                std::cin.get();
                result.button = DialogButton::Ok;
            } else {
                std::cout << "Options: ";
                for (size_t i = 0; i < dopts.buttons.size(); ++i) {
                    std::cout << "(" << i + 1 << ") "
                              << buttonToString(dopts.buttons[i]) << " ";
                }
                std::cout << "\nChoice: ";

                int choice = 1;
                if (!(std::cin >> choice)) {
                    std::cin.clear();
                    std::cin.ignore(1000, '\n');
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

    static fs::path openFileDialog(const FileDialogOptions& fdopts) {
        Tool tool = detectAvailableTool();

        if (tool == Tool::Zenity) {
            int exitCode = 0;
            std::string cmd = "zenity --file-selection --title=" +
                              escapeShellArg(fdopts.title);
            if (!fdopts.defaultPath.empty()) {
                cmd += " --filename=" +
                       escapeShellArg(fdopts.defaultPath.string());
            }
            std::string output =
                executeCommandWithOutputAndStatus(cmd, exitCode);

            // Return selected path or empty path if cancelled (exit code 1)
            if (exitCode == 0 && !output.empty()) return fs::path(output);
            if (exitCode == 1) return "";  // User explicitly cancelled
        }

        if (tool == Tool::KDialog) {
            int exitCode = 0;
            std::string startDir =
                fdopts.defaultPath.empty() ? "." : fdopts.defaultPath.string();
            std::string cmd = "kdialog --getopenfilename " +
                              escapeShellArg(startDir) + " --title " +
                              escapeShellArg(fdopts.title);
            std::string output =
                executeCommandWithOutputAndStatus(cmd, exitCode);

            if (exitCode == 0 && !output.empty()) return fs::path(output);
            if (exitCode == 1) return "";  // User explicitly cancelled
        }

        if (tool == Tool::XdgPortal) {
            bool userCancelled = false;
            fs::path resultPath = openFileViaPortal(fdopts.title, false);

            if (!resultPath.empty()) return resultPath;
            if (userCancelled) {
                return "";
            }
        }

        // Only hit this if NO dialog tool could be run at all
        std::cout << "\n[" << fdopts.title
                  << "]\nEnter file path to open (or press Enter to cancel): ";
        std::string consoleInput;
        std::getline(std::cin >> std::ws, consoleInput);
        return fs::path(consoleInput);
    }

    // -------------------------------------------------------------------------
    // Save File Dialog
    // -------------------------------------------------------------------------

    static fs::path saveFileDialog(const SaveFileDialogOptions& sfdopts) {
        Tool tool = detectAvailableTool();

        if (tool == Tool::Zenity) {
            fs::path initialPath = sfdopts.defaultPath / sfdopts.defaultName;
            std::string cmd =
                "zenity --file-selection --save --confirm-overwrite --title=" +
                escapeShellArg(sfdopts.title);
            if (!initialPath.empty()) {
                cmd += " --filename=" + escapeShellArg(initialPath.string());
            }
            std::string output = executeCommandWithOutput(cmd);
            if (!output.empty()) return fs::path(output);
        }

        if (tool == Tool::KDialog) {
            fs::path initialPath = sfdopts.defaultPath / sfdopts.defaultName;
            std::string startDir =
                initialPath.empty() ? "." : initialPath.string();
            std::string cmd = "kdialog --getsavefilename " +
                              escapeShellArg(startDir) + " --title " +
                              escapeShellArg(sfdopts.title);
            std::string output = executeCommandWithOutput(cmd);
            if (!output.empty()) return fs::path(output);
        }

        if (tool == Tool::XdgPortal) {
            fs::path resultPath = openFileViaPortal(sfdopts.title, true);
            if (!resultPath.empty()) return resultPath;
        }

        std::cout << "\n[" << sfdopts.title
                  << "]\nEnter destination path to save: ";
        std::string consoleInput;
        std::getline(std::cin >> std::ws, consoleInput);
        return fs::path(consoleInput);
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

    static fs::path openFileViaPortal(const std::string& title, bool isSave) {
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

        fs::path chosenPath = "";
        GMainLoop* mainLoop = g_main_loop_new(nullptr, FALSE);

        struct SignalContext {
            fs::path* path;
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

    static int executeCommand(const std::string& cmd) {
        return std::system(cmd.c_str());
    }

    static std::string executeCommandWithOutput(const std::string& cmd) {
        int dummyStatus = 0;
        return executeCommandWithOutputAndStatus(cmd, dummyStatus);
    }

    static std::string executeCommandWithOutputAndStatus(const std::string& cmd,
                                                         int& outExitCode) {
        std::array<char, 256> buffer;
        std::string result;

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
            if (endpos != std::string::npos) {
                result.erase(endpos + 1);
            } else {
                result.clear();
            }
        }

        return result;
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

    static std::string buttonToString(DialogButton btn) {
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

    static DialogButton stringToButton(const std::string& str) {
        if (str == "OK") return DialogButton::Ok;
        if (str == "Yes") return DialogButton::Yes;
        if (str == "Cancel") return DialogButton::Cancel;
        if (str == "No") return DialogButton::Cancel;
        return DialogButton::Custom;
    }
};
