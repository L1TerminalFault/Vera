#include "LinuxBackend.h"

#include "Association.h"
#include "Badge.h"
#include "Dialog.h"
#include "Environment.h"
#include "Integration.h"
#include "Launcher.h"
#include "Notification.h"
#include "Power.h"
#include "Sound.h"
#include "Tray.h"
#include "platform/linux/Environment.h"

NotificationResult LinuxBackend::showNotification(
    const NotificationOptions& nopts) {
    return LinuxNotification::showNotification(nopts);
}

bool LinuxBackend::closeNotification(std::uint32_t id) {
    return LinuxNotification::closeNotification(id);
}

DialogResult LinuxBackend::showDialog(const DialogOptions& dopts) {
    return LinuxDialog::showDialog(dopts);
}

std::filesystem::path LinuxBackend::openFileDialog(
    const FileDialogOptions& fdopts) {
    return LinuxDialog::openFileDialog(fdopts);
}

std::filesystem::path LinuxBackend::saveFileDialog(
    const SaveFileDialogOptions& sfdopts) {
    return LinuxDialog::saveFileDialog(sfdopts);
}

bool LinuxBackend::createTray(const TrayOptions& topts) {
    return LinuxTray::createTray(topts);
}

bool LinuxBackend::updateTray(const TrayUpdate& tupdt) {
    return LinuxTray::updateTray(tupdt);
}

bool LinuxBackend::removeTray() { return LinuxTray::removeTray(); }

bool LinuxBackend::setBadge(const BadgeOptions& bopts) {
    return LinuxBadge::setBadge(bopts);
}

bool LinuxBackend::clearBadge(const std::string& appId) {
    return LinuxBadge::clearBadge(appId);
}

bool LinuxBackend::openUrl(const UrlLaunchOptions& ulopts) {
    return LinuxLauncher::openUrl(ulopts);
}

bool LinuxBackend::openFile(const FileLaunchOptions& flopts) {
    return LinuxLauncher::openFile(flopts);
}

bool LinuxBackend::openApplication(const ApplicationLaunchOptions& alopts) {
    return LinuxLauncher::openApplication(alopts);
}

bool LinuxBackend::registerAssociation(const FileAssociation& fassocs) {
    return LinuxAssociation::registerAssociation(fassocs);
}

bool LinuxBackend::unregisterAssociation(const std::string& assoc) {
    return LinuxAssociation::unregisterAssociation(assoc);
}

bool LinuxBackend::isAssociationRegistered(const std::string& assoc) {
    return LinuxAssociation::isAssociationRegistered(assoc);
}

bool LinuxBackend::playSound(SystemSound snd) {
    return LinuxSound::playSound(snd);
}

bool LinuxBackend::preventSleep(const SleepRequest& sreq) {
    return LinuxPower::preventSleep(sreq);
}

bool LinuxBackend::allowSleep(SleepTarget sltgt) {
    return LinuxPower::allowSleep(sltgt);
}

bool LinuxBackend::requestAttention(AttentionType atype,
                                    const std::string& appId) {
    return LinuxIntegration::requestAttention(atype, appId);
}

bool LinuxBackend::setProgress(const ProgressOptions& popts) {
    return LinuxIntegration::setProgress(popts);
}

bool LinuxBackend::clearProgress(const std::string& appId) {
    return LinuxIntegration::clearProgress(appId);
}

bool LinuxBackend::setEnvironmentVariable(const std::string& name,
                                          const std::string& value) {
    return LinuxEnvironment::setEnvironmentVariable(name, value);
}

std::optional<std::string> LinuxBackend::getEnvironmentVariable(
    const std::string& name) {
    return LinuxEnvironment::getEnvironmentVariable(name);
}

bool LinuxBackend::unsetEnvironmentVariable(const std::string& name) {
    return LinuxEnvironment::unsetEnvironmentVariable(name);
}

bool LinuxBackend::addPathToEnvironment(const std::filesystem::path& pathToAdd,
                                        bool persistent) {
    return LinuxEnvironment::addPathToEnvironment(pathToAdd, persistent);
}

bool LinuxBackend::removePathFromEnvironment(
    const std::filesystem::path& pathToRemove, bool persistent) {
    return LinuxEnvironment::removePathFromEnvironment(pathToRemove,
                                                       persistent);
}
