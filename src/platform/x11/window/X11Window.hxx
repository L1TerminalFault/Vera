#pragma once

#include <X11/Xlib.h>

#include "core/window/Window.h"
#include "platform/x11/input/X11Keyboard.hxx"
#include "platform/x11/internal/X11Internal.hxx"

namespace vera::x11::window {

using namespace ::vera::x11::internal;
using namespace ::vera::x11::input;
using namespace ::vera::core::window;

class X11Window : public VeraWindow {
   public:
    X11Window(X11Context& ctx, Window xid, VeraWindowHandle handle,
              const VeraWindowInfo& info);
    ~X11Window() override;

    VeraWindowHandle getHandle() const override;
    VeraNativeHandle getNativeHandle() const override;

    void setSize(uint32_t width, uint32_t height) override;
    void setPosition(int32_t x, int32_t y) override;
    void setMinSize(uint32_t width, uint32_t height) override;
    void setMaxSize(uint32_t width, uint32_t height) override;
    VeraWindowState getState() const override;

    void show() override;
    void hide() override;
    void minimize() override;
    void maximize() override;
    void restore() override;
    void close() override;
    // Called when the WM sends WM_DELETE_WINDOW (user clicked the WM's own
    // close button / used the taskbar, not our close()). Distinct from
    // close() because close() re-sends WM_DELETE_WINDOW to itself, which
    // would loop if reused here.
    void handleWmCloseRequest();

    bool isPendingDeletion() { return m_pendingDeletion; }
    bool isRunning() { return m_isRunnig; }

    void focus() override;
    void setTitle(const std::string& title) override;
    void setFullscreen(FullScreenMode mode) override;
    void setAlwaysOnTop(bool value) override;
    void setIcon(const std::string& iconPath) override;

    void setTitlebarHitTestRegions(const VeraHitTestRegions& regions) override;

    void setResizeCallback(
        std::function<void(uint32_t, uint32_t)> callback) override;
    void setMoveCallback(
        std::function<void(int32_t, int32_t)> callback) override;
    void setCloseRequestCallback(std::function<bool()> callback) override;
    void setFocusChangeCallback(std::function<void(bool)> callback) override;
    void setDpiChangeCallback(std::function<void(float)> callback) override;

    void setKeyCallback(
        std::function<void(VeraKey, bool, bool)> callback) override;
    void setMouseButtonCallback(
        std::function<void(VeraMouseButton, bool)> callback) override;
    void setMouseMoveCallback(
        std::function<void(double, double)> callback) override;
    void setScrollCallback(
        std::function<void(double, double)> callback) override;
    void setCharCallback(std::function<void(uint32_t)> callback) override;

    void setCursorMode(VeraCursorMode mode) override;
    void setCursorShape(VeraCursorShape shape) override;

    VeraMonitorInfo getCurrentMonitor() const override;

    virtual void setJoystickButtonCallback(
        VeraJoystickButtonCallback callback) override;
    virtual void setJoystickAxisCallback(
        VeraJoystickAxisCallback callback) override;

    // --- Internal: called only by events/X11Events.cxx during event dispatch.
    // ---
    Window xid() const { return m_xid; }
    void handleXEvent(XEvent& event);

    // Retrieve the active input context for Unicode character translations
    XIC getXIC() const { return m_xic; }

   private:
    X11Context& m_ctx;
    Window m_xid;
    VeraWindowHandle m_handle;

    VeraWindowState m_state{};
    VeraHitTestRegions m_hitTestRegions;

    bool m_customTitleBar = false;
    bool m_cursorDisabled = false;

    bool m_pendingDeletion = false;
    bool m_isRunnig = false;

    XIC m_xic = nullptr;  // Managed input context handle

    KeyStateArray m_keyState{};

    std::function<void(uint32_t, uint32_t)> m_resizeCallback;
    std::function<void(int32_t, int32_t)> m_moveCallback;
    std::function<bool()> m_closeRequestCallback;
    std::function<void(bool)> m_focusChangeCallback;
    std::function<void(float)> m_dpiChangeCallback;
    std::function<void(VeraKey, bool, bool)> m_keyCallback;
    std::function<void(VeraMouseButton, bool)> m_mouseButtonCallback;
    std::function<void(double, double)> m_mouseMoveCallback;
    std::function<void(double, double)> m_scrollCallback;
    std::function<void(uint32_t)> m_charCallback;
    std::function<void(uint32_t, uint32_t, bool)> m_joyButtonCallback;
    std::function<void(uint32_t, uint32_t, float)> m_joyAxisCallback;
};

}  // namespace vera::x11::window
