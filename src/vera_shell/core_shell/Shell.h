#pragma once

#include "BackendFactory.h"

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
    nostd::Path openFileDialog(const FileDialogOptions&);
    nostd::Path saveFileDialog(const SaveFileDialogOptions&);
    bool createTray(const TrayOptions&);
    bool updateTray(const TrayUpdate&);
    bool removeTray();
    bool setBadge(const BadgeOptions&);
    bool clearBadge(const nostd::String&);
    bool openUrl(const UrlLaunchOptions&);
    bool openFile(const FileLaunchOptions&);
    bool openApplication(const ApplicationLaunchOptions&);
    bool registerAssociation(const FileAssociation&);
    bool unregisterAssociation(const nostd::String&);
    bool isAssociationRegistered(const nostd::String&);
    bool playSound(SystemSound);
    bool preventSleep(const SleepRequest&);
    bool allowSleep(SleepTarget);
    bool requestAttention(AttentionType, const nostd::String&);
    bool setProgress(const ProgressOptions&);
    bool clearProgress(const nostd::String&);

    // -------------------------------------------------------------------------
    // Environment Variables
    // -------------------------------------------------------------------------

    /// Sets or creates an environment variable for the current process.
    bool setEnvironmentVariable(const nostd::String& name,
                                const nostd::String& value);

    /// Retrieves the value of an environment variable, if it exists.
    nostd::Optional<nostd::String> getEnvironmentVariable(
        const nostd::String& name);

    /// Unsets/removes an environment variable from the current process.
    bool unsetEnvironmentVariable(const nostd::String& name);

    /// Appends a target folder to PATH.
    /// If `persistent` is true:
    ///   - Linux: Appends export to user shell config (~/.bashrc / ~/.zshrc)
    ///   - Windows: Modifies the HKCU\Environment PATH Registry key
    bool addPathToEnvironment(const nostd::Path& pathToAdd,
                              bool persistent = false);

    /// Removes a folder from PATH.
    bool removePathFromEnvironment(const nostd::Path& pathToRemove,
                                   bool persistent = false);

   private:
    std::unique_ptr<IPlatformBackend> m_backend;
};

/////////////////////////////////////////////////////////////////////////////////////////////

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

nostd::Path VeraShell::openFileDialog(const FileDialogOptions& fopts) {
    return m_backend->openFileDialog(fopts);
}

nostd::Path VeraShell::saveFileDialog(const SaveFileDialogOptions& fopts) {
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

bool VeraShell::clearBadge(const nostd::String& appId) {
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

bool VeraShell::unregisterAssociation(const nostd::String& assoc) {
    return m_backend->unregisterAssociation(assoc);
}

bool VeraShell::isAssociationRegistered(const nostd::String& assoc) {
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
                                 const nostd::String& appId) {
    return m_backend->requestAttention(atype, appId);
}

bool VeraShell::setProgress(const ProgressOptions& popts) {
    return m_backend->setProgress(popts);
}

bool VeraShell::clearProgress(const nostd::String& appId) {
    return m_backend->clearProgress(appId);
}

bool VeraShell::setEnvironmentVariable(const nostd::String& name,
                                       const nostd::String& value) {
    return m_backend->setEnvironmentVariable(name, value);
}

nostd::Optional<nostd::String> VeraShell::getEnvironmentVariable(
    const nostd::String& name) {
    return m_backend->getEnvironmentVariable(name);
}

bool VeraShell::unsetEnvironmentVariable(const nostd::String& name) {
    return m_backend->unsetEnvironmentVariable(name);
}

bool VeraShell::addPathToEnvironment(const nostd::Path& pathToAdd,
                                     bool persistent) {
    return m_backend->addPathToEnvironment(pathToAdd, persistent);
}

bool VeraShell::removePathFromEnvironment(const nostd::Path& pathToRemove,
                                          bool persistent) {
    return m_backend->removePathFromEnvironment(pathToRemove, persistent);
}
