#pragma once

#include <memory>

#include "Types.h"

class VeraShell {
   public:
    VeraShell();
    ~VeraShell();

    // -------------------------------------------------------------------------
    // Desktop & System Features
    // -------------------------------------------------------------------------

    NotificationResult showNotification(const NotificationOptions&);

    bool closeNotification(std::uint32_t);

    DialogResult showDialog(const DialogOptions&);

    std::filesystem::path openFileDialog(const FileDialogOptions&);

    std::filesystem::path saveFileDialog(const SaveFileDialogOptions&);

    bool createTray(const TrayOptions&);

    bool updateTray(const TrayUpdate&);

    bool removeTray();

    bool setBadge(const BadgeOptions&);

    bool clearBadge(const std::string&);

    bool openUrl(const UrlLaunchOptions&);

    bool openFile(const FileLaunchOptions&);

    bool openApplication(const ApplicationLaunchOptions&);

    bool registerAssociation(const FileAssociation&);

    bool unregisterAssociation(const std::string&);

    bool isAssociationRegistered(const std::string&);

    bool playSound(SystemSound);

    bool preventSleep(const SleepRequest&);

    bool allowSleep(SleepTarget);

    bool requestAttention(AttentionType, const std::string&);

    bool setProgress(const ProgressOptions&);

    bool clearProgress(const std::string&);

    // -------------------------------------------------------------------------
    // Environment Variables
    // -------------------------------------------------------------------------

    /// Sets or creates an environment variable for the current process.
    bool setEnvironmentVariable(const std::string& name,
                                const std::string& value);

    /// Retrieves the value of an environment variable, if it exists.
    std::optional<std::string> getEnvironmentVariable(const std::string& name);

    /// Unsets/removes an environment variable from the current process.
    bool unsetEnvironmentVariable(const std::string& name);

    /// Appends a target folder to PATH.
    /// If `persistent` is true:
    ///   - Linux: Appends export to user shell config (~/.bashrc / ~/.zshrc)
    ///   - Windows: Modifies the HKCU\Environment PATH Registry key
    bool addPathToEnvironment(const std::filesystem::path& pathToAdd,
                              bool persistent = false);

    /// Removes a folder from PATH.
    bool removePathFromEnvironment(const std::filesystem::path& pathToRemove,
                                   bool persistent = false);

   private:
    std::unique_ptr<IPlatformBackend> m_backend;
};
