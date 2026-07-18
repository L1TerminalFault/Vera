#pragma once

#include "core/window/Window.h"
#include "platform/wayland/internal/WaylandInternal.hxx"
#include "platform/wayland/internal/protocols/xdg-shell-client-protocol.h"

class WaylandWindow : public VeraWindow {
   public:
    WaylandWindow(WaylandContext& ctx, VeraWindowHandle handle,
                  const VeraWindowInfo& info);
    ~WaylandWindow() override;

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
    void handleWmCloseRequest();

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

    void setJoystickButtonCallback(
        VeraJoystickButtonCallback callback) override;
    void setJoystickAxisCallback(VeraJoystickAxisCallback callback) override;

    const auto& getKeyCallback() const { return m_keyCallback; }
    const auto& getCharCallback() const { return m_charCallback; }
    const auto& getMouseButtonCallback() const { return m_mouseButtonCallback; }
    const auto& getMouseMoveCallback() const { return m_mouseMoveCallback; }
    const auto& getScrollCallback() const { return m_scrollCallback; }

    // --- Internal Interface ---
    wl_surface* surface() const { return m_surface; }
    xdg_surface* xdgSurface() const { return m_xdgSurface; }
    xdg_toplevel* xdgToplevel() const { return m_xdgToplevel; }

    void handleConfigure(int32_t width, int32_t height, wl_array* states);
    void triggerResizeCallback();

    // --- Graphics Integration & Fallback ---
    WaylandContext& context() const { return m_ctx; }
    uint32_t width() const { return m_width; }
    uint32_t height() const { return m_height; }

    void markGraphicsContextActive();
    bool hasActiveGraphicsBuffer() const { return m_hasGraphicsContext; }
    void setFallbackBuffer(wl_buffer* buffer);

    bool isPendingDeletion() { return m_pendingDeletion; }
    bool isRunning() { return m_isRunning; }

   private:
    void initSurface(const VeraWindowInfo& info);
    void destroySurface();

    WaylandContext& m_ctx;
    VeraWindowHandle m_handle;

    wl_surface* m_surface = nullptr;
    xdg_surface* m_xdgSurface = nullptr;
    xdg_toplevel* m_xdgToplevel = nullptr;
    zxdg_toplevel_decoration_v1* m_decoration = nullptr;
    wl_buffer* m_fallbackBuffer = nullptr;

    VeraWindowState m_state{};
    VeraHitTestRegions m_hitTestRegions;

    bool m_customTitleBar = false;
    bool m_cursorDisabled = false;
    bool m_isResizing = false;
    bool m_pendingDeletion = false;
    bool m_isRunning = false;
    bool m_hasGraphicsContext = false;

    uint32_t m_width = 800;
    uint32_t m_height = 600;

    // Callbacks
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
