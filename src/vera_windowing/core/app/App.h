#pragma once

#include <expected>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "Types.h"

class VeraApp {
   public:
    explicit VeraApp(VeraAppInfo info);
    ~VeraApp();

    VeraApp(const VeraApp&) = delete;
    VeraApp& operator=(const VeraApp&) = delete;

    static std::unique_ptr<VeraApp> forTesting(
        VeraAppInfo info, std::unique_ptr<IBackend> backend);

    std::expected<VeraWindow*, VeraError> createWindow(
        const VeraWindowInfo& info);
    void destroyWindow(VeraWindow* window);
    VeraWindow* getWindowByHandle(VeraWindowHandle handle) const;
    size_t getWindowCount() const;
    std::vector<VeraWindow*> getAllWindows() const;

    void pollEvents();
    void waitEvents();
    void waitEventsTimeout(double timeoutSeconds);

    void setQuitRequestCallback(std::function<bool()> callback);
    void setDisplayChangeCallback(std::function<void()> callback);
    void setSystemThemeChangeCallback(
        std::function<void(VeraSystemTheme)> callback);
    void requestQuit();
    bool isQuitRequested() const;

    std::vector<VeraMonitorInfo> getMonitors() const;
    VeraMonitorInfo getPrimaryMonitor() const;
    VeraMonitorInfo getMonitorAt(int32_t x, int32_t y) const;
    std::vector<VeraDisplayModeInfo> getSupportedDisplayModes(
        const VeraMonitorInfo& monitor) const;

    bool supportsNativeDecorationHitTesting() const;

    std::string getClipboardText() const;
    void setClipboardText(const std::string& text);
    bool hasClipboardText() const;

    void setDragCallback(VeraDragCallback callback);

    VeraSystemTheme getSystemTheme() const;
    std::vector<VeraInputDeviceInfo> getInputDevices() const;
    VeraNativeHandle getNativeHandle() const;
    void applySettings(VeraSettings);

   private:
    VeraApp(VeraAppInfo info, std::unique_ptr<IBackend> backend,
            bool /*testTag*/);

    VeraAppInfo m_appInfo;
    std::unique_ptr<IBackend> m_backend;
    std::vector<std::unique_ptr<VeraWindow>> m_windows;
    std::function<bool()> m_quitRequestCallback;
    bool m_quitRequested = false;
    std::vector<VeraWindowHandle> m_pendingDestroyed;
    void drainPendingDestroyed();
};
