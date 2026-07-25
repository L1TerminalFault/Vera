#include "Shell.h"

#include <string>

#include "BackendFactory.h"

VeraShell::VeraShell() { m_backend = makePlatformBackend(); }

VeraShell::~VeraShell() = default;

NotificationResult VeraShell::showNotification(
    const NotificationOptions& nopts) {
    return m_backend->showNotification(nopts);
}

bool VeraShell::closeNotification(std::uint32_t id) {
    return m_backend->closeNotification(id);
}

DialogResult VeraShell::showDialog(const DialogOptions& opts) {
    return m_backend->showDialog(opts);
}

std::filesystem::path VeraShell::openFileDialog(
    const FileDialogOptions& fopts) {
    return m_backend->openFileDialog(fopts);
}

std::filesystem::path VeraShell::saveFileDialog(
    const SaveFileDialogOptions& fopts) {
    return m_backend->saveFileDialog(fopts);
}

bool VeraShell::createTray(const TrayOptions& topts) {
    return m_backend->createTray(topts);
}

bool VeraShell::updateTray(const TrayUpdate& updt) {
    return m_backend->updateTray(updt);
}

bool VeraShell::removeTray() { return m_backend->removeTray(); }

bool VeraShell::setBadge(const BadgeOptions& bopts) {
    return m_backend->setBadge(bopts);
}

bool VeraShell::clearBadge(const std::string& appId) {
    return m_backend->clearBadge(appId);
}

bool VeraShell::openUrl(const UrlLaunchOptions& lopts) {
    return m_backend->openUrl(lopts);
}

bool VeraShell::openFile(const FileLaunchOptions& flopts) {
    return m_backend->openFile(flopts);
}

bool VeraShell::openApplication(const ApplicationLaunchOptions& alopts) {
    return m_backend->openApplication(alopts);
}

bool VeraShell::registerAssociation(const FileAssociation& fassoc) {
    return m_backend->registerAssociation(fassoc);
}

bool VeraShell::unregisterAssociation(const std::string& assoc) {
    return m_backend->unregisterAssociation(assoc);
}

bool VeraShell::isAssociationRegistered(const std::string& assoc) {
    return m_backend->isAssociationRegistered(assoc);
}

bool VeraShell::playSound(SystemSound snd) { return m_backend->playSound(snd); }

bool VeraShell::preventSleep(const SleepRequest& slreq) {
    return m_backend->preventSleep(slreq);
}

bool VeraShell::allowSleep(SleepTarget stgt) {
    return m_backend->allowSleep(stgt);
}

bool VeraShell::requestAttention(AttentionType atype,
                                 const std::string& appId) {
    return m_backend->requestAttention(atype, appId);
}

bool VeraShell::setProgress(const ProgressOptions& popts) {
    return m_backend->setProgress(popts);
}

bool VeraShell::clearProgress(const std::string& appId) {
    return m_backend->clearProgress(appId);
}

bool VeraShell::setEnvironmentVariable(const std::string& name,
                                       const std::string& value) {
    return m_backend->setEnvironmentVariable(name, value);
}

std::optional<std::string> VeraShell::getEnvironmentVariable(
    const std::string& name) {
    return m_backend->getEnvironmentVariable(name);
}

bool VeraShell::unsetEnvironmentVariable(const std::string& name) {
    return m_backend->unsetEnvironmentVariable(name);
}

bool VeraShell::addPathToEnvironment(const std::filesystem::path& pathToAdd,
                                     bool persistent) {
    return m_backend->addPathToEnvironment(pathToAdd, persistent);
}

bool VeraShell::removePathFromEnvironment(
    const std::filesystem::path& pathToRemove, bool persistent) {
    return m_backend->removePathFromEnvironment(pathToRemove, persistent);
}
