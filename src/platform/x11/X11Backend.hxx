#pragma once

#include "core/app/Types.h"
#include "platform/x11/internal/X11Internal.hxx"

class X11Backend : public IBackend {
   public:
    X11Backend() = default;
    ~X11Backend() override;

    bool initialize(const VeraAppInfo& info);

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
    void applySettings(VeraSettings) override;

   private:
    mutable X11Context m_ctx;
    bool m_hasXInput2 = false;
    int m_xinput2Opcode = 0;

    std::function<bool()> m_quitRequestCallback;
    std::function<void()> m_displayChangeCallback;
};
