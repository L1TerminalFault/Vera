#pragma once
#include <expected>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "core/app/err.h"
#include "core/app/types.h"
#include "core/window/window.h"

class VeraApp {
   public:
    explicit VeraApp(VeraAppInfo info);
    ~VeraApp();

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

    bool supportsNativeDecorationHitTesting() const;

    std::string getClipboardText() const;
    void setClipboardText(const std::string& text);
    bool hasClipboardText() const;

    void setDragCallback(VeraDragCallback callback);

    std::vector<VeraDisplayModeInfo> getSupportedDisplayModes(
        const VeraMonitorInfo& monitor) const;

    VeraSystemTheme getSystemTheme() const;

    std::vector<VeraInputDeviceInfo> getInputDevices() const;
    VeraNativeHandle getNativeHandle() const;

   private:
    VeraAppInfo m_appInfo;
    std::vector<std::unique_ptr<VeraWindow>> m_windows;
};
