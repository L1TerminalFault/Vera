#pragma once

#include "Backend.h"

class VeraApp {
   public:
    explicit VeraApp(VeraAppInfo info);
    ~VeraApp();

    VeraApp(const VeraApp&) = delete;
    VeraApp& operator=(const VeraApp&) = delete;

    void initBackend();
    static nostd::UniquePtr<VeraApp> forTesting(
        VeraAppInfo info, nostd::UniquePtr<IBackend> backend);

    nostd::Expected<VeraWindow*, VeraError> createWindow(
        const VeraWindowInfo& info);
    void destroyWindow(VeraWindow* window);
    VeraWindow* getWindowByHandle(VeraWindowHandle handle) const;
    size_t getWindowCount() const;
    nostd::Vector<VeraWindow*> getAllWindows() const;

    void pollEvents();
    void waitEvents();
    void waitEventsTimeout(double timeoutSeconds);

    void setQuitRequestCallback(nostd::Function<bool()> callback);
    void setDisplayChangeCallback(nostd::Function<void()> callback);
    void setSystemThemeChangeCallback(
        nostd::Function<void(VeraSystemTheme)> callback);
    void requestQuit();
    bool isQuitRequested() const;

    nostd::Vector<VeraMonitorInfo> getMonitors() const;
    VeraMonitorInfo getPrimaryMonitor() const;
    VeraMonitorInfo getMonitorAt(int32_t x, int32_t y) const;
    nostd::Vector<VeraDisplayModeInfo> getSupportedDisplayModes(
        const VeraMonitorInfo& monitor) const;

    bool supportsNativeDecorationHitTesting() const;

    VeraStringView getClipboardText() const;
    void setClipboardText(const char* text);
    bool hasClipboardText() const;

    void setDragCallback(VeraDragCallback callback);

    VeraSystemTheme getSystemTheme() const;
    nostd::Vector<VeraInputDeviceInfo> getInputDevices() const;
    VeraNativeHandle getNativeHandle() const;
    void applySettings(const VeraSettings&);
    void setCursorShape(VeraCursorShape) const;

   private:
    VeraApp(VeraAppInfo info, nostd::UniquePtr<IBackend> backend,
            bool /*testTag*/);

    VeraAppInfo m_appInfo;
    nostd::UniquePtr<IBackend> m_backend;
    nostd::Vector<nostd::UniquePtr<VeraWindow>> m_windows;
    nostd::Function<bool()> m_quitRequestCallback;
    bool m_quitRequested = false;
    nostd::Vector<VeraWindowHandle> m_pendingDestroyed;
    void drainPendingDestroyed();
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void VeraApp::initBackend() {
    if (!m_backend) {
        fprintf(
            stderr,
            "[VeraApp Fatal] Platform backend initialization failed!\n"
            "Check if $DISPLAY (X11) or $WAYLAND_DISPLAY is set correctly.\n");
        std::exit(EXIT_FAILURE);
    }

    m_backend->setQuitRequestCallback([this]() -> bool {
        m_quitRequested = true;
        if (m_quitRequestCallback) return m_quitRequestCallback();
        return true;
    });
}

VeraApp::VeraApp(VeraAppInfo info) : m_appInfo(info), m_backend(create(info)) {
    initBackend();
}

VeraApp::VeraApp(VeraAppInfo info, nostd::UniquePtr<IBackend> backend, bool)
    : m_appInfo(info), m_backend(std::move(backend)) {
    initBackend();
}

VeraApp::~VeraApp() = default;

nostd::UniquePtr<VeraApp> VeraApp::forTesting(
    VeraAppInfo info, nostd::UniquePtr<IBackend> backend) {
    return nostd::UniquePtr<VeraApp>(
        new VeraApp(info, std::move(backend), true));
}

nostd::Expected<VeraWindow*, VeraError> VeraApp::createWindow(
    const VeraWindowInfo& info) {
    if (!m_backend) {
        return nostd::Expected<VeraWindow*, VeraError>(
            nostd::Unexpected(VeraError{VeraErrorType::BackendInitFailed,
                                        "No platform backend available"}));
    }
    auto result = m_backend->createWindow(info);
    if (!result) {
        return nostd::Expected<VeraWindow*, VeraError>(
            nostd::Unexpected(result.error()));
    }

    VeraWindow* raw = result->get();
    VeraWindowHandle handle = raw->getHandle();

    raw->setDestroyedNotifier([this, handle](VeraWindowHandle) {
        m_pendingDestroyed.push_back(handle);
    });

    m_windows.push_back(std::move(*result));
    return nostd::Expected<VeraWindow*, VeraError>(raw);
}

void VeraApp::destroyWindow(VeraWindow* window) {
    if (!window) return;

    m_windows.erase_if(
        [window](const auto& win) { return win.get() == window; });
}

VeraWindow* VeraApp::getWindowByHandle(VeraWindowHandle handle) const {
    for (const auto& w : m_windows) {
        if (w->getHandle() == handle) return w.get();
    }
    return nullptr;
}

size_t VeraApp::getWindowCount() const { return m_windows.size(); }

nostd::Vector<VeraWindow*> VeraApp::getAllWindows() const {
    nostd::Vector<VeraWindow*> out;
    out.reserve(m_windows.size());
    for (const auto& w : m_windows) out.push_back(w.get());
    return out;
}

void VeraApp::pollEvents() {
    if (m_backend) m_backend->pollEvents();
    drainPendingDestroyed();
}
void VeraApp::waitEvents() {
    if (m_backend) m_backend->waitEvents();
    drainPendingDestroyed();
}
void VeraApp::waitEventsTimeout(double timeoutSeconds) {
    if (m_backend) m_backend->waitEventsTimeout(timeoutSeconds);
    drainPendingDestroyed();
}

void VeraApp::drainPendingDestroyed() {
    for (VeraWindowHandle handle : m_pendingDestroyed) {
        m_windows.erase_if(
            [handle](const auto& win) { return win->getHandle() == handle; });
    }
    m_pendingDestroyed.clear();
}

void VeraApp::setQuitRequestCallback(nostd::Function<bool()> callback) {
    m_quitRequestCallback = std::move(callback);
}
void VeraApp::setDisplayChangeCallback(nostd::Function<void()> callback) {
    if (m_backend) m_backend->setDisplayChangeCallback(std::move(callback));
}
void VeraApp::setSystemThemeChangeCallback(
    nostd::Function<void(VeraSystemTheme)> callback) {
    if (m_backend) m_backend->setSystemThemeChangeCallback(std::move(callback));
}
void VeraApp::requestQuit() { m_quitRequested = true; }
bool VeraApp::isQuitRequested() const { return m_quitRequested; }

nostd::Vector<VeraMonitorInfo> VeraApp::getMonitors() const {
    return m_backend ? m_backend->getMonitors()
                     : nostd::Vector<VeraMonitorInfo>{};
}
VeraMonitorInfo VeraApp::getPrimaryMonitor() const {
    return m_backend ? m_backend->getPrimaryMonitor() : VeraMonitorInfo{};
}
VeraMonitorInfo VeraApp::getMonitorAt(int32_t x, int32_t y) const {
    return m_backend ? m_backend->getMonitorAt(x, y) : VeraMonitorInfo{};
}
nostd::Vector<VeraDisplayModeInfo> VeraApp::getSupportedDisplayModes(
    const VeraMonitorInfo& monitor) const {
    return m_backend ? m_backend->getSupportedDisplayModes(monitor)
                     : nostd::Vector<VeraDisplayModeInfo>{};
}

bool VeraApp::supportsNativeDecorationHitTesting() const {
    return m_backend && m_backend->supportsNativeDecorationHitTesting();
}

VeraStringView VeraApp::getClipboardText() const {
    if (m_backend) {
        return m_backend->getClipboardText();
    }
    return {};
}
void VeraApp::setClipboardText(const char* text) {
    if (m_backend) m_backend->setClipboardText(text);
}
bool VeraApp::hasClipboardText() const {
    return m_backend && m_backend->hasClipboardText();
}

void VeraApp::setDragCallback(VeraDragCallback callback) {
    if (m_backend) m_backend->setDragCallback(std::move(callback));
}

VeraSystemTheme VeraApp::getSystemTheme() const {
    return m_backend ? m_backend->getSystemTheme() : VeraSystemTheme::Unknown;
}
nostd::Vector<VeraInputDeviceInfo> VeraApp::getInputDevices() const {
    return m_backend ? m_backend->getInputDevices()
                     : nostd::Vector<VeraInputDeviceInfo>{};
}
VeraNativeHandle VeraApp::getNativeHandle() const {
    return m_backend ? m_backend->getNativeHandle() : VeraNativeHandle{};
}

void VeraApp::applySettings(const VeraSettings& settings) {
    if (!m_backend) {
        printf("[VeraApp] Error: m_backend is null\n");
        return;
    }
    m_backend->applySettings(settings);
}

void VeraApp::setCursorShape(VeraCursorShape cursh) const {
    m_backend->setCursorShape(cursh);
}
