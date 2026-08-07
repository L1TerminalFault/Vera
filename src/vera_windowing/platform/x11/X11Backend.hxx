#pragma once

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/cursorfont.h>

#include "core/app/Types.h"

class X11Window;

struct X11Atoms {
    Atom wmProtocols = 0;
    Atom wmDeleteWindow = 0;
    Atom wmState = 0;

    Atom netWmState = 0;
    Atom netWmStateFullscreen = 0;
    Atom netWmStateMaximizedHorz = 0;
    Atom netWmStateMaximizedVert = 0;
    Atom netWmStateAbove = 0;
    Atom netWmStateHidden = 0;
    Atom netWmName = 0;
    Atom netWmIcon = 0;
    Atom netWmIconName = 0;
    Atom netWmWindowType = 0;
    Atom netWmWindowTypeNormal = 0;
    Atom netWmPid = 0;
    Atom netWmMoveresize = 0;
    Atom netWorkarea = 0;
    Atom netCurrentDesktop = 0;
    Atom netWmSyncRequest = 0;

    Atom motifWmHints = 0;

    Atom utf8String = 0;
    Atom clipboard = 0;
    Atom targets = 0;
    Atom multiple = 0;
    Atom incr = 0;

    Atom xdndAware = 0;
    Atom xdndEnter = 0;
    Atom xdndPosition = 0;
    Atom xdndStatus = 0;
    Atom xdndLeave = 0;
    Atom xdndDrop = 0;
    Atom xdndFinished = 0;
    Atom xdndSelection = 0;
    Atom xdndActionCopy = 0;
    Atom textUriList = 0;

    Atom xSettingsSettings = 0;
};

//////////////////////////////////////////////////////////////////////////////////////

struct KeyRepeatStateX11 {
    Window window;
    uint32_t key;
    uint32_t scanCode;
    VeraKey veraKey;
    uint64_t nextRepeatMs;  // Replaced std::chrono::steady_clock::time_point
                            // with raw monotonic ms
};

constexpr size_t MAX_JOYSTICK_PATH = 256;

struct JoystickDeviceX11 {
    int fd = -1;
    char devicePath[MAX_JOYSTICK_PATH] = {};
    VeraJoystickState state{};
};

struct X11WindowMapEntry {
    Window xid = 0;
    X11Window* window = nullptr;
};

struct X11Context {
    Display* display = nullptr;
    int screen = 0;
    Window root = 0;
    X11Atoms atoms;

    XIM xim = nullptr;

    // Replaced std::unordered_map with flat array/vector mappings
    nostd::Vector<X11WindowMapEntry> windowsByXid;

    uint32_t keyRepeatRate = 0;
    uint32_t keyRepeatDelay = 0;

    // Key repeat array (rarely contains more than a few simultaneous keys)
    nostd::Vector<KeyRepeatStateX11> pressedKeys;

    Window clipboardOwnerWindow = 0;

    uint64_t nextHandleValue = 1;

    bool keyStates[static_cast<size_t>(VeraKey::Count)] = {false};
    bool mouseButtonStates[static_cast<size_t>(VeraMouseButton::Count)] = {
        false};

    VeraWindowHandle allocateHandle() {
        return VeraWindowHandle{nextHandleValue++};
    }

    // Allocate space for up to 16 controller slots directly managed in context
    // memory
    nostd::Vector<JoystickDeviceX11> joysticks =
        nostd::Vector<JoystickDeviceX11>(16);
};

////////////////////////////////////////////////////////////////////////////////////////

class X11Backend : public IBackend {
   public:
    X11Backend() = default;
    ~X11Backend() override;

    bool initialize(const VeraAppInfo& info);

    nostd::Expected<nostd::UniquePtr<VeraWindow>, VeraError> createWindow(
        const VeraWindowInfo& info) override;

    void pollEvents() override;
    void waitEvents() override;
    void waitEventsTimeout(double timeoutSeconds) override;

    void setQuitRequestCallback(nostd::Function<bool()> callback) override;
    void setDisplayChangeCallback(nostd::Function<void()> callback) override;
    void setSystemThemeChangeCallback(
        nostd::Function<void(VeraSystemTheme)> callback) override;

    nostd::Vector<VeraMonitorInfo> getMonitors() const override;
    VeraMonitorInfo getPrimaryMonitor() const override;
    VeraMonitorInfo getMonitorAt(int32_t x, int32_t y) const override;
    nostd::Vector<VeraDisplayModeInfo> getSupportedDisplayModes(
        const VeraMonitorInfo& monitor) const override;

    bool supportsNativeDecorationHitTesting() const override;

    VeraStringView getClipboardText() const override;
    void setClipboardText(const char* text) override;
    bool hasClipboardText() const override;

    void setDragCallback(VeraDragCallback callback) override;

    VeraSystemTheme getSystemTheme() const override;
    nostd::Vector<VeraInputDeviceInfo> getInputDevices() const override;
    VeraNativeHandle getNativeHandle() const override;
    void applySettings(const VeraSettings&) override;
    void setCursorShape(VeraCursorShape) override;

   private:
    mutable X11Context m_ctx;
    bool m_hasXInput2 = false;
    int m_xinput2Opcode = 0;

    nostd::Function<bool()> m_quitRequestCallback;
    nostd::Function<void()> m_displayChangeCallback;
};

///////////////////////////////////////////////////////////////////////////////////////////////////////

bool initializeXInputX11(X11Context& ctx, int& outOpcode);

nostd::Vector<VeraInputDeviceInfo> enumerateInputDevicesX11(X11Context& ctx);

///////////////////////////////////////////////////////////////////////////////////////////////////////

void applyCursorModeX11(X11Context& ctx, Window window, VeraCursorMode mode);

void shutdownCursorX11(X11Context& ctx);

void applyCursorShapeX11(X11Context& ctx, Window window, VeraCursorShape shape);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void initializeClipboardX11(X11Context& ctx);

void setClipboardTextX11(X11Context& ctx, const char* text);

VeraStringView getClipboardTextX11(X11Context& ctx);

bool hasClipboardTextX11(X11Context& ctx);

void handleClipboardSelectionRequestX11(X11Context& ctx,
                                        XSelectionRequestEvent& request);

void handleClipboardSelectionClearX11(X11Context&, XSelectionClearEvent&);

////////////////////////////////////////////////////////////////////////////////////////////////

void setCallback(VeraDragCallback callback);

void handleClientMessage(X11Context& ctx, VeraWindow* window,
                         XClientMessageEvent& event);

void handleSelectionNotify(X11Context& ctx, VeraWindow* window,
                           XSelectionEvent& event);

////////////////////////////////////////////////////////////////////////////////////////////////////////////

void initializeThemeX11(X11Context& ctx);

VeraSystemTheme getCurrentThemeX11(X11Context& ctx);

void setThemeChangeCallbackX11(nostd::Function<void(VeraSystemTheme)> callback);

void handleThemePropertyNotifyX11(X11Context& ctx, XPropertyEvent& event);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void pollEventsX11(X11Context& ctx,
                   const nostd::Function<bool()>& quitRequestCallback,
                   const nostd::Function<void()>& displayChangeCallback);

void waitForEventsX11(X11Context& ctx,
                      const nostd::Function<bool()>& quitRequestCallback,
                      const nostd::Function<void()>& displayChangeCallback);

void waitForEventsWithTimeoutX11(
    X11Context& ctx, double timeoutSeconds,
    const nostd::Function<bool()>& quitRequestCallback,
    const nostd::Function<void()>& displayChangeCallback);

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void setJoystickButtonCallbackX11(
    nostd::Function<void(uint32_t, uint32_t, bool)> cb);

void setJoystickAxisCallbackX11(
    nostd::Function<void(uint32_t, uint32_t, float)> cb);

void initializeJoystickX11(X11Context& ctx);

void updateJoystickX11(X11Context& ctx);

VeraJoystickState getJoystickStateX11(X11Context& ctx, uint32_t joystickId);

void shutdownJoystickX11(X11Context& ctx);

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

using KeyStateArray = std::array<bool, static_cast<size_t>(VeraKey::Count)>;

void handleKeyPressX11(
    X11Context& ctx, XKeyEvent& event, KeyStateArray& state,
    const nostd::Function<void(VeraKey, bool, bool)>& keyCallback,
    const nostd::Function<void(const char*)>& charCallback);

void handleKeyReleaseX11(
    X11Context& ctx, XKeyEvent& event, KeyStateArray& state,
    const nostd::Function<void(VeraKey, bool, bool)>& keyCallback);

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void initializeXKBX11(X11Context& ctx);

VeraKey convertKeyEventToVeraKeyX11(X11Context& ctx, XKeyEvent& event);

uint32_t convertKeyEventToCodepointX11(X11Context& ctx, XKeyEvent& event);

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool initializeMonitorX11(X11Context& ctx);

nostd::Vector<VeraMonitorInfo> getMonitorsX11(X11Context& ctx);

VeraMonitorInfo getPrimaryMonitorX11(X11Context& ctx);

VeraMonitorInfo getMonitorAtCoordinateXYX11(X11Context& ctx, int32_t x,
                                            int32_t y);

nostd::Vector<VeraDisplayModeInfo> getSupportedDisplayModesX11(
    X11Context& ctx, const VeraMonitorInfo& monitor);

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool initializeXRandRX11(X11Context& ctx);

float queryDpiScaleX11(X11Context& ctx);

nostd::Vector<VeraMonitorInfo> queryMonitorsX11(X11Context& ctx);

nostd::Vector<VeraDisplayModeInfo> queryDisplayModesX11(
    X11Context& ctx, const VeraMonitorInfo& monitor);

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void setDecoratedX11(X11Context& ctx, Window window, bool decorated);

void handleTitlebarButtonPressX11(X11Context& ctx, Window window,
                                  const VeraHitTestRegions& regions,
                                  int32_t clickX, int32_t clickY);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void applyFullscreenX11(X11Context& ctx, Window window, FullScreenMode mode);

////////////////////////////////////////////////////////////////////////////////////////////////////////////

void setTitleX11(X11Context& ctx, Window window, const char* title);

void setIconX11(X11Context& ctx, Window window, const char* iconPath);

void setSizeHintsX11(X11Context& ctx, Window window, uint32_t minWidth,
                     uint32_t minHeight, uint32_t maxWidth, uint32_t maxHeight,
                     bool resizable);

void setNetWmStateX11(X11Context& ctx, Window window, Atom state1, Atom state2,
                      bool add);

void setAlwaysOnTopX11(X11Context& ctx, Window window, bool value);

void setWindowTypeX11(X11Context& ctx, Window window);

void setPidX11(X11Context& ctx, Window window);

bool hasNetWmStateX11(X11Context& ctx, Window window, Atom state);

//////////////////////////////////////////////////////////////////////////////////////////////////////////////

void internAtomsX11(X11Context& ctx);

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////

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

    void handleWmCloseRequest();

    bool isPendingDeletion() { return m_pendingDeletion; }
    bool isRunning() { return m_isRunnig; }

    void focus() override;
    void setTitle(const char* title) override;
    void setFullscreen(FullScreenMode mode) override;
    void setAlwaysOnTop(bool value) override;
    void setIcon(const char* iconPath) override;

    void setTitlebarHitTestRegions(const VeraHitTestRegions& regions) override;

    void setResizeCallback(
        nostd::Function<void(uint32_t, uint32_t)> callback) override;
    void setMoveCallback(
        nostd::Function<void(int32_t, int32_t)> callback) override;
    void setCloseRequestCallback(nostd::Function<bool()> callback) override;
    void setFocusChangeCallback(nostd::Function<void(bool)> callback) override;
    void setDpiChangeCallback(nostd::Function<void(float)> callback) override;

    void setKeyCallback(
        nostd::Function<void(VeraKey, bool, bool)> callback) override;
    void setMouseButtonCallback(
        nostd::Function<void(VeraMouseButton, bool)> callback) override;
    void setMouseMoveCallback(
        nostd::Function<void(double, double)> callback) override;
    void setScrollCallback(
        nostd::Function<void(double, double)> callback) override;
    void setCharCallback(nostd::Function<void(const char*)> callback) override;

    void setCursorMode(VeraCursorMode mode) override;
    void setCursorShape(VeraCursorShape shape) override;

    VeraMonitorInfo getCurrentMonitor() const override;

    void setJoystickButtonCallback(
        VeraJoystickButtonCallback callback) override;
    void setJoystickAxisCallback(VeraJoystickAxisCallback callback) override;

    void setDestructionCallback(
        nostd::Function<void(VeraWindow*)> callback) override;
    bool isPressed(const VeraPressable&) const override;

    const auto& getKeyCallback() const { return m_keyCallback; }
    const auto& getCharCallback() const { return m_charCallback; }
    const auto& getMouseButtonCallback() const { return m_mouseButtonCallback; }
    const auto& getMouseMoveCallback() const { return m_mouseMoveCallback; }
    const auto& getScrollCallback() const { return m_scrollCallback; }

    Window xid() const { return m_xid; }
    void handleXEvent(XEvent& event);

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

    XIC m_xic = nullptr;

    KeyStateArray m_keyState{};

    nostd::Function<void(uint32_t, uint32_t)> m_resizeCallback;
    nostd::Function<void(int32_t, int32_t)> m_moveCallback;
    nostd::Function<bool()> m_closeRequestCallback;
    nostd::Function<void(bool)> m_focusChangeCallback;
    nostd::Function<void(float)> m_dpiChangeCallback;
    nostd::Function<void(VeraKey, bool, bool)> m_keyCallback;
    nostd::Function<void(VeraMouseButton, bool)> m_mouseButtonCallback;
    nostd::Function<void(double, double)> m_mouseMoveCallback;
    nostd::Function<void(double, double)> m_scrollCallback;
    nostd::Function<void(const char*)> m_charCallback;
    nostd::Function<void(uint32_t, uint32_t, bool)> m_joyButtonCallback;
    nostd::Function<void(uint32_t, uint32_t, float)> m_joyAxisCallback;
};

// Helper function to locate an X11Window by XID
X11Window* findWindowByXid(const X11Context& ctx, Window xid);
