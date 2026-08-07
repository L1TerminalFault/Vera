#include "LinuxBackend.h"

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

nostd::Path LinuxBackend::openFileDialog(const FileDialogOptions& fdopts) {
    return LinuxDialog::openFileDialog(fdopts);
}

nostd::Path LinuxBackend::saveFileDialog(const SaveFileDialogOptions& sfdopts) {
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

bool LinuxBackend::clearBadge(const nostd::String& appId) {
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

bool LinuxBackend::unregisterAssociation(const nostd::String& assoc) {
    return LinuxAssociation::unregisterAssociation(assoc);
}

bool LinuxBackend::isAssociationRegistered(const nostd::String& assoc) {
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
                                    const nostd::String& appId) {
    return LinuxIntegration::requestAttention(atype, appId);
}

bool LinuxBackend::setProgress(const ProgressOptions& popts) {
    return LinuxIntegration::setProgress(popts);
}

bool LinuxBackend::clearProgress(const nostd::String& appId) {
    return LinuxIntegration::clearProgress(appId);
}

bool LinuxBackend::setEnvironmentVariable(const nostd::String& name,
                                          const nostd::String& value) {
    return LinuxEnvironment::setEnvironmentVariable(name, value);
}

nostd::Optional<nostd::String> LinuxBackend::getEnvironmentVariable(
    const nostd::String& name) {
    return LinuxEnvironment::getEnvironmentVariable(name);
}

bool LinuxBackend::unsetEnvironmentVariable(const nostd::String& name) {
    return LinuxEnvironment::unsetEnvironmentVariable(name);
}

bool LinuxBackend::addPathToEnvironment(const nostd::Path& pathToAdd,
                                        bool persistent) {
    return LinuxEnvironment::addPathToEnvironment(pathToAdd, persistent);
}

bool LinuxBackend::removePathFromEnvironment(const nostd::Path& pathToRemove,
                                             bool persistent) {
    return LinuxEnvironment::removePathFromEnvironment(pathToRemove,
                                                       persistent);
}
