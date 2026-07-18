#include "core/app/App.h"

#include "core/platform/PlatformFactory.h"

VeraApp::VeraApp(VeraAppInfo info) : m_appInfo(info), m_backend(create(info)) {
    if (m_backend) {
        m_backend->setQuitRequestCallback([this]() -> bool {
            m_quitRequested = true;
            if (m_quitRequestCallback) return m_quitRequestCallback();
            return true;
        });
    }
}

VeraApp::VeraApp(VeraAppInfo info, std::unique_ptr<IPlatformBackend> backend,
                 bool)
    : m_appInfo(info), m_backend(std::move(backend)) {}

VeraApp::~VeraApp() = default;

std::unique_ptr<VeraApp> VeraApp::forTesting(
    VeraAppInfo info, std::unique_ptr<IPlatformBackend> backend) {
    return std::unique_ptr<VeraApp>(
        new VeraApp(info, std::move(backend), true));
}

std::expected<VeraWindow*, VeraError> VeraApp::createWindow(
    const VeraWindowInfo& info) {
    if (!m_backend) {
        return std::unexpected(VeraError{VeraErrorType::BackendInitFailed,
                                         "No platform backend available"});
    }
    auto result = m_backend->createWindow(info);
    if (!result) return std::unexpected(result.error());

    VeraWindow* raw = result->get();
    VeraWindowHandle handle = raw->getHandle();

    raw->setDestroyedNotifier([this, handle](VeraWindowHandle) {
        m_pendingDestroyed.push_back(handle);
    });

    m_windows.push_back(std::move(*result));
    return raw;
}

void VeraApp::destroyWindow(VeraWindow* window) {
    std::erase_if(m_windows,
                  [window](const auto& win) { return win.get() == window; });
}

VeraWindow* VeraApp::getWindowByHandle(VeraWindowHandle handle) const {
    for (const auto& w : m_windows) {
        if (w->getHandle() == handle) return w.get();
    }
    return nullptr;
}

size_t VeraApp::getWindowCount() const { return m_windows.size(); }

std::vector<VeraWindow*> VeraApp::getAllWindows() const {
    std::vector<VeraWindow*> out;
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
        std::erase_if(m_windows,
                      [handle](const auto& win) { return win->getHandle() == handle; });
    }
    m_pendingDestroyed.clear();
}

void VeraApp::setQuitRequestCallback(std::function<bool()> callback) {
    m_quitRequestCallback = std::move(callback);
}
void VeraApp::setDisplayChangeCallback(std::function<void()> callback) {
    if (m_backend) m_backend->setDisplayChangeCallback(std::move(callback));
}
void VeraApp::setSystemThemeChangeCallback(
    std::function<void(VeraSystemTheme)> callback) {
    if (m_backend) m_backend->setSystemThemeChangeCallback(std::move(callback));
}
void VeraApp::requestQuit() { m_quitRequested = true; }
bool VeraApp::isQuitRequested() const { return m_quitRequested; }

std::vector<VeraMonitorInfo> VeraApp::getMonitors() const {
    return m_backend ? m_backend->getMonitors()
                     : std::vector<VeraMonitorInfo>{};
}
VeraMonitorInfo VeraApp::getPrimaryMonitor() const {
    return m_backend ? m_backend->getPrimaryMonitor() : VeraMonitorInfo{};
}
VeraMonitorInfo VeraApp::getMonitorAt(int32_t x, int32_t y) const {
    return m_backend ? m_backend->getMonitorAt(x, y) : VeraMonitorInfo{};
}
std::vector<VeraDisplayModeInfo> VeraApp::getSupportedDisplayModes(
    const VeraMonitorInfo& monitor) const {
    return m_backend ? m_backend->getSupportedDisplayModes(monitor)
                     : std::vector<VeraDisplayModeInfo>{};
}

bool VeraApp::supportsNativeDecorationHitTesting() const {
    return m_backend && m_backend->supportsNativeDecorationHitTesting();
}

std::string VeraApp::getClipboardText() const {
    return m_backend ? m_backend->getClipboardText() : std::string{};
}
void VeraApp::setClipboardText(const std::string& text) {
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
std::vector<VeraInputDeviceInfo> VeraApp::getInputDevices() const {
    return m_backend ? m_backend->getInputDevices()
                     : std::vector<VeraInputDeviceInfo>{};
}
VeraNativeHandle VeraApp::getNativeHandle() const {
    return m_backend ? m_backend->getNativeHandle() : VeraNativeHandle{};
}
