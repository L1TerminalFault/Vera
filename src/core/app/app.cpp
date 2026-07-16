#include "app.h"
#include "platform/window/vera_win32.h"
#include "platform\intern.h"
#include <expected>

VeraApp::VeraApp(VeraAppInfo info) : m_appInfo(info) {}
VeraApp::~VeraApp() = default;


std::expected<VeraWindow*, VeraError> VeraApp::createWindow(
    const VeraWindowInfo& info) {
    auto window = std::make_unique<VeraWin32Window>(info);

    if (!window->getNativeHandle().hwnd) {
        return std::unexpected(
            VeraError{VeraErrorType::WindowCreationFailed, "Failed to create window"});
    }

    VeraWindow* rawPtr = window.get();
    m_windows.push_back(std::move(window));

    return rawPtr;
}

void VeraApp::destroyWindow(VeraWindow* window) {
    std::erase_if(m_windows,
                  [window](const auto& win) { return win.get() == window; });
}

size_t VeraApp::getWindowCount() const { return m_windows.size(); }

void VeraApp::pollEvents() {
    vera::internal::pollPlatformEvents();
}