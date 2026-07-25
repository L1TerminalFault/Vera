#include "../../core_shell/Types.h"

class LinuxBackend : public IPlatformBackend {
   public:
    LinuxBackend() = default;
    ~LinuxBackend() = default;

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

    bool setEnvironmentVariable(const std::string& name,
                                const std::string& value) override;

    std::optional<std::string> getEnvironmentVariable(
        const std::string& name) override;

    bool unsetEnvironmentVariable(const std::string& name) override;

    bool addPathToEnvironment(const std::filesystem::path& pathToAdd,
                              bool persistent) override;

    bool removePathFromEnvironment(const std::filesystem::path& pathToRemove,
                                   bool persistent) override;
};
