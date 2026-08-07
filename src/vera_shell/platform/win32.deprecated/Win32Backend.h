#include "../../core_shell/Types.h"

class Win32Backend : public IPlatformBackend {
   public:
    Win32Backend() = default;
    ~Win32Backend() override = default;

    NotificationResult showNotification(const NotificationOptions&) override;

    bool closeNotification(std::uint32_t) override;

    DialogResult showDialog(const DialogOptions&) override;

    std::filesystem::path openFileDialog(const FileDialogOptions&) override;

    std::filesystem::path saveFileDialog(const SaveFileDialogOptions&) override;

    bool createTray(const TrayOptions&) override;

    bool updateTray(const TrayUpdate&) override;

    bool removeTray() override;

    bool setBadge(const BadgeOptions&) override;

    bool clearBadge(const std::string&) override;

    bool openUrl(const UrlLaunchOptions&) override;

    bool openFile(const FileLaunchOptions&) override;

    bool openApplication(const ApplicationLaunchOptions&) override;

    bool registerAssociation(const FileAssociation&) override;

    bool unregisterAssociation(const std::string&) override;

    bool isAssociationRegistered(const std::string&) override;

    bool playSound(SystemSound) override;

    bool preventSleep(const SleepRequest&) override;

    bool allowSleep(SleepTarget) override;

    bool requestAttention(AttentionType, const std::string&) override;

    bool setProgress(const ProgressOptions&) override;

    bool clearProgress(const std::string&) override;
};
