#include "core/window/Window.h"
#include "core/window/WindowTypes.h"

#if defined(VERA_PLATFORM_WIN32)
#pragma once

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <atomic>

class VeraWin32Window : public VeraWindow {
   public:
    explicit VeraWin32Window(const VeraWindowInfo& info);
    ~VeraWin32Window() override;

    VeraWindowHandle getHandle() const override { return m_handle; }
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

    // Input callbacks stubs
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
        VeraJoystickButtonCallback callback) override {
        m_joystick_button_callback = std::move(callback);
    }
    void setJoystickAxisCallback(
        VeraJoystickAxisCallback callback) override {
        m_joystick_axis_callback = std::move(callback);
    }
    void setDestructionCallback(std::function<void(VeraWindow*)> callback) override {
        m_destruction_callback = std::move(callback);
    }

    
    VeraJoystickButtonCallback m_joystick_button_callback{nullptr};
    VeraJoystickAxisCallback m_joystick_axis_callback{nullptr};

   private:
    void createNativeWindow(const VeraWindowInfo& info, DWORD style,
                            DWORD ex_style, int x, int y, int width,
                            int height);

    static LRESULT CALLBACK windowProcRouter(HWND hwnd, UINT msg, WPARAM wparam,
                                             LPARAM lparam);

    VeraWindowHandle m_handle;
    HWND m_hwnd = nullptr;
    std::optional<uint32_t> m_min_width;
    std::optional<uint32_t> m_min_height;
    std::optional<uint32_t> m_max_width;
    std::optional<uint32_t> m_max_height;

    RECT m_saved_window_rect = {0, 0, 0, 0};
    DWORD m_saved_window_style = 0;
    DWORD m_saved_window_ex_style = 0;
    bool m_is_fullscreen_cached = false;
    std::optional<VeraHitTestRegions> m_hit_test_regions;

    std::function<void(uint32_t, uint32_t)> m_resize_callback;
    std::function<void(int32_t, int32_t)> m_move_callback;
    std::function<bool()> m_close_request_callback;
    std::function<void(bool)> m_focus_change_callback;
    std::function<void(float)> m_dpi_change_callback;
    std::function<void(VeraWindow*)> m_destruction_callback{ nullptr };

    std::function<void(VeraKey, bool, bool)> m_key_callback;
    std::function<void(VeraMouseButton, bool)> m_mouse_button_callback;
    std::function<void(double, double)> m_mouse_move_callback;
    std::function<void(double, double)> m_scroll_callback;
    std::function<void(uint32_t)> m_char_callback;
    LRESULT handleMessage(UINT msg, WPARAM wparam, LPARAM lparam);

    VeraCursorMode m_cursor_mode = VeraCursorMode::Normal;
    VeraCursorShape m_cursor_shape = VeraCursorShape::Arrow;
    HCURSOR m_current_hcursor = nullptr;

    bool m_mouse_tracked = false;
};

#endif
