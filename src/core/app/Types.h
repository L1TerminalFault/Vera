#pragma once

#include <cstdint>
#include <expected>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <variant>

// Theme related
enum class VeraThemeMode : uint8_t { Light = 0, Dark };

enum class VeraLinuxProtocol : uint8_t {
    Auto = 0,
    Wayland,
    X11,
};

enum class VeraSystemTheme { Light, Dark, Unknown };

// Info related
struct VeraAppInfo {
    bool enablePlatformDebugging = false;
    VeraLinuxProtocol preferedLinuxProtocol = VeraLinuxProtocol::Auto;
};

// Settings related
struct KeyRepeatSettings {
    uint32_t delayMs;
    uint32_t rate;
};

struct VeraSettings {
    KeyRepeatSettings keyRepeatSettings;
};

// Error related
enum class VeraErrorType {
    WindowCreationFailed,
    RemovedNonExistingWindow,
    BackendInitFailed,
    UnsupportedOperation,
    DefaultError
};

struct VeraError {
    VeraErrorType type;
    std::string info;
};

// Window related
struct VeraRect {
    uint32_t x = 0;
    uint32_t y = 0;
    uint32_t width = 0;
    uint32_t height = 0;
};

struct VeraWindowState {
    uint32_t width, height;
    int32_t x, y;
    bool isMinimized, isMaximized, isFullscreen, isFocused, isVisible;
};

enum class FullScreenMode { Windowed, Borderless, Exclusive };

struct VeraHitTestRegions {
    std::optional<VeraRect> dragRegion;
    std::optional<VeraRect> minimizeButton;
    std::optional<VeraRect> maximizeButton;
    std::optional<VeraRect> closeButton;
};

struct VeraWindowHandle {
    uint64_t value = 0;

    bool operator==(const VeraWindowHandle&) const = default;
    bool operator!=(const VeraWindowHandle&) const = default;
    explicit operator bool() const { return value != 0; }

    static constexpr VeraWindowHandle invalid() { return VeraWindowHandle{0}; }
};

struct VeraWindowInfo {
    std::optional<uint32_t> x;
    std::optional<uint32_t> y;

    uint32_t width;
    uint32_t height;

    uint32_t minWidth = 0, minHeight = 0;
    uint32_t maxWidth = 0, maxHeight = 0;

    std::string title = "Vera Window";

    bool resizable = true;
    bool decorated = true;

    bool startMaximized = false;
    bool startMinimized = false;
    FullScreenMode fullscreenMode = FullScreenMode::Windowed;

    bool startVisible = true;
    bool focusOnShow = true;
    bool alwaysOnTop = false;
    bool customTitleBar = false;
    uint32_t titleBarHeight = 32;

    int monitorIndex = 0;
    bool centerOnMonitor = true;
    bool transparentFramebuffer = false;
    std::string iconPath;
};

template <>
struct std::hash<VeraWindowHandle> {
    size_t operator()(const VeraWindowHandle& h) const noexcept {
        return std::hash<uint64_t>{}(h.value);
    }
};

// Key related
enum class VeraKey : uint16_t {
    Unknown = 0,

    A,
    B,
    C,
    D,
    E,
    F,
    G,
    H,
    I,
    J,
    K,
    L,
    M,
    N,
    O,
    P,
    Q,
    R,
    S,
    T,
    U,
    V,
    W,
    X,
    Y,
    Z,

    Num0,
    Num1,
    Num2,
    Num3,
    Num4,
    Num5,
    Num6,
    Num7,
    Num8,
    Num9,

    F1,
    F2,
    F3,
    F4,
    F5,
    F6,
    F7,
    F8,
    F9,
    F10,
    F11,
    F12,
    F13,
    F14,
    F15,
    F16,
    F17,
    F18,
    F19,
    F20,
    F21,
    F22,
    F23,
    F24,

    Space,
    Enter,
    Escape,
    Tab,
    Backspace,
    Insert,
    Delete,
    Home,
    End,
    PageUp,
    PageDown,

    Left,
    Right,
    Up,
    Down,

    LeftShift,
    RightShift,
    LeftCtrl,
    RightCtrl,
    LeftAlt,
    RightAlt,
    LeftSuper,
    RightSuper,

    CapsLock,
    ScrollLock,
    NumLock,
    PrintScreen,
    Pause,
    Menu,

    Apostrophe,
    Comma,
    Minus,
    Period,
    Slash,
    Semicolon,
    Equal,
    LeftBracket,
    Backslash,
    RightBracket,
    GraveAccent,

    KP0,
    KP1,
    KP2,
    KP3,
    KP4,
    KP5,
    KP6,
    KP7,
    KP8,
    KP9,
    KPDecimal,
    KPDivide,
    KPMultiply,
    KPSubtract,
    KPAdd,
    KPEnter,
    KPEqual,

    VolumeUp,
    VolumeDown,
    Mute,

    Count
};

// Mouse related
enum class VeraMouseButton : uint8_t {
    Left,
    Right,
    Middle,
    VeraButton4,
    VeraButton5,
    Count
};

enum class VeraCursorMode : uint8_t { Normal = 0, Hidden, Disabled };

enum class VeraCursorShape : uint8_t {
    Arrow = 0,
    IBeam,
    Crosshair,
    Hand,
    HResize,
    VResize,
    CornerResizeNWSE,
    CornerResizeNESW,
    NotAllowed,
    Count
};

// Input related
struct VeraInputDeviceInfo {
    std::string name;
    bool connected;
};

// Monitor related
struct VeraMonitorInfo {
    std::string name;
    int32_t x, y;
    int32_t workAreaX, workAreaY;
    uint32_t workAreaWidth, workAreaHeight;
    float dpiScale;
    uint32_t refreshRateHz;
    bool isPrimary;
    std::optional<uint32_t> physicalWidthMm, physicalHeightMm;
};

struct VeraDisplayModeInfo {
    uint32_t width, height;
    uint32_t refreshRateHz;
    uint32_t bitsPerPixel;
};

// Native handle
struct VeraNativeHandle {
    void* hwnd = nullptr;
    void* display = nullptr;
    uint64_t x11Window = 0;
    void* waylandSurface = nullptr;
};

// Joystick related
struct VeraJoystickState {
    std::string name;
    bool connected = false;
    std::vector<float> axes;
    std::vector<bool> buttons;
};

using VeraJoystickButtonCallback =
    std::function<void(uint32_t joyId, uint32_t btn, bool pressed)>;
using VeraJoystickAxisCallback =
    std::function<void(uint32_t joyId, uint32_t axis, float val)>;

enum class VeraJoystickButton : uint8_t {
    // Matched by same opcodes
    // Face Buttons
    //  - PlayStation
    Cross,
    Circle,
    Square,
    Triangle,
    //  - Xbox
    XboxA,
    XboxB,
    XboxX,
    XboxY,

    // Bumpers
    //  - PlayStation
    L1,
    R1,
    //  - Xbox
    XboxLB,
    XboxRB,

    // Triggers (Digital/Click states)
    //  - PlayStation
    L2,
    R2,
    //  - Xbox
    XboxLT,
    XboxRT,

    // Stick Clicks
    //  - PlayStation
    L3,
    R3,
    //  - Xbox
    XboxLS,
    XboxRS,

    // Menu and System Buttons
    //  - PlayStation
    Share,
    Options,
    PS,
    //  - Xbox
    XboxBack,   // View
    XboxStart,  // Menu
    XboxGuide,

    // Platform-Specific Exclusives
    //  - PlayStation
    Touchpad,  // PlayStation: Touchpad Click (Unmapped on Xbox)
    //  - Xbox
    XboxShare,  // Dedicated Series X|S Capture button (Unmapped on PS)

    // Directional D-Pad Same on both platforms
    DpadUp,
    DpadDown,
    DpadLeft,
    DpadRight,

    Count
};

// Joining all pressables
using VeraPressable =
    std::variant<VeraKey, VeraMouseButton, VeraJoystickButton>;

// Window interface - leaving it cuz the backend needs to have some context to
// call createWindow
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
    virtual bool isPressed(VeraPressable) const = 0;

    virtual void setCursorMode(VeraCursorMode mode) = 0;
    virtual void setCursorShape(VeraCursorShape shape) = 0;

    virtual VeraMonitorInfo getCurrentMonitor() const = 0;

    void setDestroyedNotifier(std::function<void(VeraWindowHandle)> notifier) {
        m_destroyedNotifier = std::move(notifier);
    }

    virtual void setJoystickButtonCallback(
        VeraJoystickButtonCallback callback) = 0;
    virtual void setJoystickAxisCallback(VeraJoystickAxisCallback callback) = 0;

    virtual void setDestructionCallback(
        std::function<void(VeraWindow*)> callback) = 0;

   protected:
    void notifyDestroyed() {
        if (m_destroyedNotifier) {
            m_destroyedNotifier(getHandle());
        }
    }

   private:
    std::function<void(VeraWindowHandle)> m_destroyedNotifier;
};

// Drag related
enum class VeraDragAction { Enter, Over, Drop, Leave };

struct VeraDragEvent {
    VeraDragAction action;
    VeraWindow* window;
    int32_t x, y;
    std::vector<std::string> paths;
};

using VeraDragCallback = std::function<bool(const VeraDragEvent&)>;

// Backend Interface
class IBackend {
   public:
    virtual ~IBackend() = default;

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
    virtual void applySettings(VeraSettings) = 0;
};
