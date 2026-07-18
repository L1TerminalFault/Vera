#pragma once

#include "core/app/AppSettings.h"
#include "core/platform/IPlatformBackend.h"
#include "platform/wayland/internal/WaylandInternal.hxx"

class WaylandBackend : public IPlatformBackend {
   public:
    WaylandBackend() = default;
    ~WaylandBackend() override;

    bool initialize(const VeraAppInfo& info);
    void shutdown();

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

    WaylandContext& getContext() { return m_ctx; }
    const WaylandContext& getContext() const { return m_ctx; }

   private:
    mutable WaylandContext m_ctx;

    std::function<bool()> m_quitRequestCallback;
    std::function<void()> m_displayChangeCallback;
    std::function<void(VeraSystemTheme)> m_systemThemeChangeCallback;
};
