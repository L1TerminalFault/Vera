#include "Win32Backend.h"

#include "Association.h"
#include "Badge.h"
#include "Dialog.h"
#include "Integration.h"
#include "Launcher.h"
#include "Notification.h"
#include "Power.h"
#include "Sound.h"
#include "Tray.h"

NotificationResult Win32Backend::showNotification(
    const NotificationOptions& nopts) {
    return Win32Notification::showNotification(nopts);
}

bool Win32Backend::closeNotification(std::uint32_t id) {
    return Win32Notification::closeNotification(id);
}

DialogResult Win32Backend::showDialog(const DialogOptions& dopts) {
    return Win32Dialog::showDialog(dopts);
}

std::filesystem::path Win32Backend::openFileDialog(
    const FileDialogOptions& fdopts) {
    return Win32Dialog::openFileDialog(fdopts);
}

std::filesystem::path Win32Backend::saveFileDialog(
    const SaveFileDialogOptions& sfdopts) {
    return Win32Dialog::saveFileDialog(sfdopts);
}

bool Win32Backend::createTray(const TrayOptions& topts) {
    return Win32Tray::createTray(topts);
}

bool Win32Backend::updateTray(const TrayUpdate& tupdt) {
    return Win32Tray::updateTray(tupdt);
}

bool Win32Backend::removeTray() { return Win32Tray::removeTray(); }

bool Win32Backend::setBadge(const BadgeOptions& bopts) {
    return Win32Badge::setBadge(bopts);
}

bool Win32Backend::clearBadge(const std::string& appId) {
    return Win32Badge::clearBadge(appId);
}

bool Win32Backend::openUrl(const UrlLaunchOptions& ulopts) {
    return Win32Launcher::openUrl(ulopts);
}

bool Win32Backend::openFile(const FileLaunchOptions& flopts) {
    return Win32Launcher::openFile(flopts);
}

bool Win32Backend::openApplication(const ApplicationLaunchOptions& alopts) {
    return Win32Launcher::openApplication(alopts);
}

bool Win32Backend::registerAssociation(const FileAssociation& fassocs) {
    return Win32Association::registerAssociation(fassocs);
}

bool Win32Backend::unregisterAssociation(const std::string& assoc) {
    return Win32Association::unregisterAssociation(assoc);
}

bool Win32Backend::isAssociationRegistered(const std::string& assoc) {
    return Win32Association::isAssociationRegistered(assoc);
}

bool Win32Backend::playSound(SystemSound snd) {
    return Win32Sound::playSound(snd);
}

bool Win32Backend::preventSleep(const SleepRequest& sreq) {
    return Win32Power::preventSleep(sreq);
}

bool Win32Backend::allowSleep(SleepTarget sltgt) {
    return Win32Power::allowSleep(sltgt);
}

bool Win32Backend::requestAttention(AttentionType atype,
                                    const std::string& appId) {
    return Win32Integration::requestAttention(atype, appId);
}

bool Win32Backend::setProgress(const ProgressOptions& popts) {
    return Win32Integration::setProgress(popts);
}

bool Win32Backend::clearProgress(const std::string& appId) {
    return Win32Integration::clearProgress(appId);
}
