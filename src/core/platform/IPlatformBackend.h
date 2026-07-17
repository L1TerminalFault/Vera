#pragma once

#include <expected>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "core/app/AppInfo.h"
#include "core/app/Error.h"
#include "core/monitor/Monitor.h"
#include "core/platform/NativeHandle.h"
#include "core/window/DragDrop.h"
#include "core/window/Window.h"

class IPlatformBackend {
   public:
    virtual ~IPlatformBackend() = default;

    virtual std::expected<std::unique_ptr<VeraWindow>, VeraError> createWindow(
        const VeraWindowInfo& info) = 0;

    virtual void pollEvents() = 0;
    virtual void waitEvents() = 0;
    virtual void waitEventsTimeout(double timeoutSeconds) = 0;

    virtual void setQuitRequestCallback(std::function<bool()> callback) = 0;
    virtual void setDisplayChangeCallback(std::function<void()> callback) = 0;
    virtual void setSystemThemeChangeCallback(
        std::function<void(VeraSystemTheme)> callback) = 0;

    virtual std::vector<VeraMonitorInfo> getMonitors() const = 0;
    virtual VeraMonitorInfo getPrimaryMonitor() const = 0;
    virtual VeraMonitorInfo getMonitorAt(int32_t x, int32_t y) const = 0;
    virtual std::vector<VeraDisplayModeInfo> getSupportedDisplayModes(
        const VeraMonitorInfo& monitor) const = 0;

    virtual bool supportsNativeDecorationHitTesting() const = 0;

    virtual std::string getClipboardText() const = 0;
    virtual void setClipboardText(const std::string& text) = 0;
    virtual bool hasClipboardText() const = 0;

    virtual void setDragCallback(VeraDragCallback callback) = 0;

    virtual VeraSystemTheme getSystemTheme() const = 0;
    virtual std::vector<VeraInputDeviceInfo> getInputDevices() const = 0;
    virtual VeraNativeHandle getNativeHandle() const = 0;
};
