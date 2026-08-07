#pragma once

#include "../../shared/nostd.h"

enum class NotificationUrgency { Low = 0, Normal = 1, Critical = 2 };

struct NotificationOptions {
    nostd::String title;
    nostd::String body;
    nostd::String icon;
    nostd::String appName = "App";
    uint32_t replacesId = 0;
    int32_t expireTimeoutMs = -1;
    NotificationUrgency urgency = NotificationUrgency::Normal;
    nostd::Vector<std::pair<nostd::String, nostd::String>> actions;
};

struct NotificationResult {
    bool success = false;
    std::uint32_t notificationId = 0;
};

using Progress = std::uint32_t;

#undef Status
#undef Success
#undef None

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
    nostd::String customButtonText;  // Populated if a custom button was pressed
};

struct DialogOptions {
    nostd::String title;
    nostd::String message;

    DialogIcon icon = DialogIcon::None;

    nostd::Vector<DialogButton> buttons;
};

struct FileDialogOptions {
    nostd::String title = "Open File";
    nostd::Path defaultPath;
    nostd::Vector<nostd::String> filters;  // e.g., {"*.png", "*.jpg"}
};

struct SaveFileDialogOptions {
    nostd::String title = "Save File";
    nostd::Path defaultPath;
    nostd::String defaultName;
    nostd::Vector<nostd::String> filters;
};

struct TrayMenuItem {
    nostd::String id;
    nostd::String label;
    bool enabled = true;
    bool checked = false;
    bool isSeparator = false;
    nostd::Function<void()> onClick;
};

struct TrayOptions {
    nostd::String id = "app-tray";
    nostd::String title = "Application";
    nostd::String iconName = "application-x-executable";
    nostd::String tooltip = "Application running";
    nostd::Vector<TrayMenuItem> menuItems;
};

struct TrayUpdate {
    nostd::String title;
    nostd::String iconName;
    nostd::String tooltip;
    nostd::Vector<TrayMenuItem> menuItems;
};

struct BadgeOptions {
    int count;
    bool countVisible;
    nostd::String appId;
};

struct UrlLaunchOptions {
    nostd::String url;
};

struct FileLaunchOptions {
    nostd::Path filePath;
};

struct ApplicationLaunchOptions {
    nostd::String appId;
    nostd::String command;
    nostd::Vector<nostd::String> args;
};

struct FileAssociation {
    nostd::String extension;

    nostd::String mimeType;

    nostd::String description;

    nostd::String command;
};

enum class SystemSound { Notification, Warning, Error, Question, Success };

enum class SleepTarget { Screen, System, All };

struct SleepRequest {
    SleepTarget target = SleepTarget::System;
    nostd::String reason = "Application busy";
};

struct ProgressOptions {
    double progress;
    bool progressVisible;
    nostd::String appId;
};

enum class AttentionType { Informational, Critical };

class IPlatformBackend {
   public:
    virtual ~IPlatformBackend() = default;
    virtual NotificationResult showNotification(const NotificationOptions&) = 0;
    virtual bool closeNotification(std::uint32_t id) = 0;
    virtual DialogResult showDialog(const DialogOptions&) = 0;
    virtual nostd::Path openFileDialog(const FileDialogOptions&) = 0;
    virtual nostd::Path saveFileDialog(const SaveFileDialogOptions&) = 0;
    virtual bool createTray(const TrayOptions&) = 0;
    virtual bool updateTray(const TrayUpdate&) = 0;
    virtual bool removeTray() = 0;
    virtual bool setBadge(const BadgeOptions&) = 0;
    virtual bool clearBadge(const nostd::String&) = 0;
    virtual bool openUrl(const UrlLaunchOptions&) = 0;
    virtual bool openFile(const FileLaunchOptions&) = 0;
    virtual bool openApplication(const ApplicationLaunchOptions&) = 0;
    virtual bool registerAssociation(const FileAssociation&) = 0;
    virtual bool unregisterAssociation(const nostd::String&) = 0;
    virtual bool isAssociationRegistered(const nostd::String&) = 0;
    virtual bool playSound(SystemSound) = 0;
    virtual bool preventSleep(const SleepRequest&) = 0;
    virtual bool allowSleep(SleepTarget) = 0;
    virtual bool requestAttention(AttentionType, const nostd::String&) = 0;
    virtual bool setProgress(const ProgressOptions&) = 0;
    virtual bool clearProgress(const nostd::String&) = 0;
    virtual bool setEnvironmentVariable(const nostd::String& name,
                                        const nostd::String& value) = 0;
    virtual nostd::Optional<nostd::String> getEnvironmentVariable(
        const nostd::String& name) = 0;
    virtual bool unsetEnvironmentVariable(const nostd::String& name) = 0;
    virtual bool addPathToEnvironment(const nostd::Path& pathToAdd,
                                      bool persistent) = 0;
    virtual bool removePathFromEnvironment(const nostd::Path& pathToRemove,
                                           bool persistent) = 0;
};
