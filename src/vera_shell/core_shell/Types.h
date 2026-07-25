#pragma once

#include <filesystem>
#include <functional>
#include <string>
#include <vector>

enum class NotificationUrgency { Low = 0, Normal = 1, Critical = 2 };

struct NotificationOptions {
    std::string title;
    std::string body;
    std::string icon;
    std::string appName = "App";
    std::uint32_t replacesId = 0;
    int32_t expireTimeoutMs = -1;
    NotificationUrgency urgency = NotificationUrgency::Normal;
    std::vector<std::pair<std::string, std::string>> actions;
};

struct NotificationResult {
    bool success = false;
    std::uint32_t notificationId = 0;
};

using Progress = std::uint32_t;

enum class Status {
    Success,
    Failure,
    Unsupported,
    Cancelled,
    PermissionDenied
};

enum class DialogIcon { None, Info, Warning, Error, Question };

enum class DialogButton { Ok, Cancel, Yes, No, Custom };

struct DialogResult {
    DialogButton button = DialogButton::Cancel;
    std::string customButtonText;  // Populated if a custom button was pressed
};

struct DialogOptions {
    std::string title;
    std::string message;

    DialogIcon icon = DialogIcon::None;

    std::vector<DialogButton> buttons;
};

struct FileDialogOptions {
    std::string title = "Open File";
    std::filesystem::path defaultPath;
    std::vector<std::string> filters;  // e.g., {"*.png", "*.jpg"}
};

struct SaveFileDialogOptions {
    std::string title = "Save File";
    std::filesystem::path defaultPath;
    std::string defaultName;
    std::vector<std::string> filters;
};

struct TrayMenuItem {
    std::string id;
    std::string label;
    bool enabled = true;
    bool checked = false;
    bool isSeparator = false;
    std::function<void()> onClick;
};

struct TrayOptions {
    std::string id = "app-tray";
    std::string title = "Application";
    std::string iconName = "application-x-executable";
    std::string tooltip = "Application running";
    std::vector<TrayMenuItem> menuItems;
};

struct TrayUpdate {
    std::string title;
    std::string iconName;
    std::string tooltip;
    std::vector<TrayMenuItem> menuItems;
};

struct BadgeOptions {
    int count;
    bool countVisible;
    std::string appId;
};

struct UrlLaunchOptions {
    std::string url;
};

struct FileLaunchOptions {
    std::filesystem::path filePath;
};

struct ApplicationLaunchOptions {
    std::string appId;
    std::string command;
    std::vector<std::string> args;
};

struct FileAssociation {
    std::string extension;

    std::string mimeType;

    std::string description;

    std::string command;
};

enum class SystemSound { Notification, Warning, Error, Question, Success };

enum class SleepTarget { Screen, System, All };

struct SleepRequest {
    SleepTarget target = SleepTarget::System;
    std::string reason = "Application busy";
};

struct ProgressOptions {
    double progress;
    bool progressVisible;
    std::string appId;
};

enum class AttentionType { Informational, Critical };

class IPlatformBackend {
   public:
    virtual ~IPlatformBackend() = default;

    virtual NotificationResult showNotification(const NotificationOptions&) = 0;

    virtual bool closeNotification(std::uint32_t id) = 0;

    virtual DialogResult showDialog(const DialogOptions&) = 0;

    virtual std::filesystem::path openFileDialog(const FileDialogOptions&) = 0;

    virtual std::filesystem::path saveFileDialog(
        const SaveFileDialogOptions&) = 0;

    virtual bool createTray(const TrayOptions&) = 0;

    virtual bool updateTray(const TrayUpdate&) = 0;

    virtual bool removeTray() = 0;

    virtual bool setBadge(const BadgeOptions&) = 0;

    virtual bool clearBadge(const std::string&) = 0;

    virtual bool openUrl(const UrlLaunchOptions&) = 0;

    virtual bool openFile(const FileLaunchOptions&) = 0;

    virtual bool openApplication(const ApplicationLaunchOptions&) = 0;

    virtual bool registerAssociation(const FileAssociation&) = 0;

    virtual bool unregisterAssociation(const std::string&) = 0;

    virtual bool isAssociationRegistered(const std::string&) = 0;

    virtual bool playSound(SystemSound) = 0;

    virtual bool preventSleep(const SleepRequest&) = 0;

    virtual bool allowSleep(SleepTarget) = 0;

    virtual bool requestAttention(AttentionType, const std::string&) = 0;

    virtual bool setProgress(const ProgressOptions&) = 0;

    virtual bool clearProgress(const std::string&) = 0;

    virtual bool setEnvironmentVariable(const std::string& name,
                                        const std::string& value) = 0;

    virtual std::optional<std::string> getEnvironmentVariable(
        const std::string& name) = 0;

    virtual bool unsetEnvironmentVariable(const std::string& name) = 0;

    virtual bool addPathToEnvironment(const std::filesystem::path& pathToAdd,
                                      bool persistent) = 0;

    virtual bool removePathFromEnvironment(
        const std::filesystem::path& pathToRemove, bool persistent) = 0;
};
