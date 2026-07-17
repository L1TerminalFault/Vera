#pragma once

#include "core/platform/IPlatformBackend.h"

namespace vera::win32 {

class Win32Backend : public IPlatformBackend {
   public:
    Win32Backend() = default;
    ~Win32Backend() override = default;

    std::expected<std::unique_ptr<VeraWindow>, VeraError> createWindow(
        const VeraWindowInfo& info) override;

    void pollEvents() override;
    void waitEvents() override;
    void waitEventsTimeout(double timeoutSeconds) override;

    void setQuitRequestCallback(std::function<bool()> callback) override;
    void setDisplayChangeCallback(std::function<void()> callback) override;
    void setSystemThemeChangeCallback(
        std::function<void(VeraSystemTheme)> callback) override;

    std::vector<VeraMonitorInfo> getMonitors() const override;
    VeraMonitorInfo getPrimaryMonitor() const override;
    VeraMonitorInfo getMonitorAt(int32_t x, int32_t y) const override;
    std::vector<VeraDisplayModeInfo> getSupportedDisplayModes(
        const VeraMonitorInfo& monitor) const override;

    bool supportsNativeDecorationHitTesting() const override;

    std::string getClipboardText() const override;
    void setClipboardText(const std::string& text) override;
    bool hasClipboardText() const override;

    void setDragCallback(VeraDragCallback callback) override;

    VeraSystemTheme getSystemTheme() const override;
    std::vector<VeraInputDeviceInfo> getInputDevices() const override;
    VeraNativeHandle getNativeHandle() const override;

     virtual void setJoystickButtonCallback(
        VeraJoystickButtonCallback callback) override {};
    virtual void setJoystickAxisCallback(
        VeraJoystickAxisCallback callback) override {};

   private:
    std::function<bool()> m_quitRequestCallback;
    std::function<void()> m_displayChangeCallback;
    std::function<void(VeraSystemTheme)> m_themeChangeCallback;
    VeraDragCallback m_dragCallback;
};

}  // namespace vera::win32
