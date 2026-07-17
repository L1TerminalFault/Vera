#pragma once
#include <functional>
#include <string>

#include "core/input/Joystick.h"
#include "core/input/Keys.h"
#include "core/input/Mouse.h"
#include "core/monitor/Monitor.h"
#include "core/platform/NativeHandle.h"
#include "core/window/WindowTypes.h"

namespace vera::core::window {

using namespace input;
using namespace monitor;

class VeraWindow {
   public:
    virtual ~VeraWindow() = default;

    virtual VeraWindowHandle getHandle() const = 0;
    virtual VeraNativeHandle getNativeHandle() const = 0;

    virtual void setSize(uint32_t width, uint32_t height) = 0;
    virtual void setPosition(int32_t x, int32_t y) = 0;
    virtual void setMinSize(uint32_t width, uint32_t height) = 0;
    virtual void setMaxSize(uint32_t width, uint32_t height) = 0;
    virtual VeraWindowState getState() const = 0;

    virtual void show() = 0;
    virtual void hide() = 0;
    virtual void minimize() = 0;
    virtual void maximize() = 0;
    virtual void restore() = 0;
    virtual void close() = 0;

    virtual void focus() = 0;
    virtual void setTitle(const std::string& title) = 0;
    virtual void setFullscreen(FullScreenMode mode) = 0;
    virtual void setAlwaysOnTop(bool value) = 0;
    virtual void setIcon(const std::string& iconPath) = 0;

    virtual void setTitlebarHitTestRegions(
        const VeraHitTestRegions& regions) = 0;

    virtual void setResizeCallback(
        std::function<void(uint32_t width, uint32_t height)> callback) = 0;
    virtual void setMoveCallback(
        std::function<void(int32_t x, int32_t y)> callback) = 0;
    virtual void setCloseRequestCallback(std::function<bool()> callback) = 0;
    virtual void setFocusChangeCallback(
        std::function<void(bool focused)> callback) = 0;
    virtual void setDpiChangeCallback(
        std::function<void(float newScale)> callback) = 0;

    virtual void setKeyCallback(
        std::function<void(VeraKey key, bool pressed, bool repeat)>
            callback) = 0;
    virtual void setMouseButtonCallback(
        std::function<void(VeraMouseButton button, bool pressed)> callback) = 0;
    virtual void setMouseMoveCallback(
        std::function<void(double x, double y)> callback) = 0;
    virtual void setScrollCallback(
        std::function<void(double xOffset, double yOffset)> callback) = 0;
    virtual void setCharCallback(
        std::function<void(uint32_t codepoint)> callback) = 0;

    virtual void setCursorMode(VeraCursorMode mode) = 0;
    virtual void setCursorShape(VeraCursorShape shape) = 0;

    virtual VeraMonitorInfo getCurrentMonitor() const = 0;

    virtual void setJoystickButtonCallback(
        VeraJoystickButtonCallback callback) = 0;
    virtual void setJoystickAxisCallback(VeraJoystickAxisCallback callback) = 0;
};

}  // namespace vera::core::window
