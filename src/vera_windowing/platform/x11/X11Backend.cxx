#include "platform/x11/X11Backend.hxx"

#include <X11/X.h>
#include <X11/Xlocale.h>
#include <X11/extensions/XInput2.h>

bool X11Backend::initialize(const VeraAppInfo& info) {
    XInitThreads();

    std::setlocale(LC_ALL, "");
    XSetLocaleModifiers("");

    m_ctx.display = XOpenDisplay(nullptr);
    if (!m_ctx.display) return false;

    m_ctx.xim = XOpenIM(m_ctx.display, nullptr, nullptr, nullptr);
    if (!m_ctx.xim) {
        XSetLocaleModifiers("@im=none");
        m_ctx.xim = XOpenIM(m_ctx.display, nullptr, nullptr, nullptr);
    }

    m_ctx.screen = DefaultScreen(m_ctx.display);
    m_ctx.root = RootWindow(m_ctx.display, m_ctx.screen);

    internAtomsX11(m_ctx);
    initializeXKBX11(m_ctx);
    m_hasXInput2 = initializeXInputX11(m_ctx, m_xinput2Opcode);
    initializeMonitorX11(m_ctx);
    initializeClipboardX11(m_ctx);
    initializeThemeX11(m_ctx);

    initializeJoystickX11(m_ctx);

    if (info.enablePlatformDebugging) {
        XSynchronize(m_ctx.display, True);
    }

    return true;
}

X11Backend::~X11Backend() {
    if (m_ctx.display) {
        shutdownCursorX11(m_ctx);
        if (m_ctx.clipboardOwnerWindow) {
            XDestroyWindow(m_ctx.display, m_ctx.clipboardOwnerWindow);
        }

        if (m_ctx.xim) {
            XCloseIM(m_ctx.xim);
            m_ctx.xim = nullptr;
        }

        shutdownJoystickX11(m_ctx);
        XCloseDisplay(m_ctx.display);
    }
}

nostd::Expected<nostd::UniquePtr<VeraWindow>, VeraError>
X11Backend::createWindow(const VeraWindowInfo& info) {
    VeraMonitorInfo targetMonitor = getPrimaryMonitorX11(m_ctx);
    auto monitors = getMonitorsX11(m_ctx);
    if (info.monitorIndex >= 0 &&
        static_cast<size_t>(info.monitorIndex) < monitors.size()) {
        targetMonitor = monitors[static_cast<size_t>(info.monitorIndex)];
    }

    int32_t x, y;
    if (info.x && info.y) {
        x = static_cast<int32_t>(*info.x);
        y = static_cast<int32_t>(*info.y);
    } else if (info.centerOnMonitor) {
        x = targetMonitor.workAreaX +
            (static_cast<int32_t>(targetMonitor.workAreaWidth) -
             static_cast<int32_t>(info.width)) /
                2;
        y = targetMonitor.workAreaY +
            (static_cast<int32_t>(targetMonitor.workAreaHeight) -
             static_cast<int32_t>(info.height)) /
                2;
    } else {
        x = targetMonitor.workAreaX;
        y = targetMonitor.workAreaY;
    }

    XSetWindowAttributes attributes{};
    attributes.background_pixel = WhitePixel(m_ctx.display, m_ctx.screen);
    attributes.override_redirect = False;
    ulong valueMask = CWBackPixel;

    int depth = DefaultDepth(m_ctx.display, m_ctx.screen);
    Visual* visual = DefaultVisual(m_ctx.display, m_ctx.screen);

    if (info.transparentFramebuffer) {
        XVisualInfo visualInfoTemplate{};
        visualInfoTemplate.screen = m_ctx.screen;
        visualInfoTemplate.depth = 32;
        visualInfoTemplate.c_class = TrueColor;
        int matchCount = 0;
        XVisualInfo* matches = XGetVisualInfo(
            m_ctx.display, VisualScreenMask | VisualDepthMask | VisualClassMask,
            &visualInfoTemplate, &matchCount);
        if (matches && matchCount > 0) {
            depth = matches[0].depth;
            visual = matches[0].visual;
            attributes.colormap =
                XCreateColormap(m_ctx.display, m_ctx.root, visual, AllocNone);
            valueMask |= CWColormap | CWBorderPixel;
            attributes.border_pixel = 0;
            XFree(matches);
        }
    }

    Window xid =
        XCreateWindow(m_ctx.display, m_ctx.root, x, y, info.width, info.height,
                      0, depth, InputOutput, visual, valueMask, &attributes);
    if (!xid) {
        return nostd::Unexpected(VeraError{VeraErrorType::WindowCreationFailed,
                                           "XCreateWindow failed"});
    }

    VeraWindowHandle handle = m_ctx.allocateHandle();
    auto win = nostd::makeUnique<X11Window>(m_ctx, xid, handle, info);
    return nostd::UniquePtr<VeraWindow>(win.release());
}

void X11Backend::pollEvents() {
    updateJoystickX11(m_ctx);
    pollEventsX11(m_ctx, m_quitRequestCallback, m_displayChangeCallback);

    for (size_t i = 0; i < m_ctx.windowsByXid.size();) {
        X11Window* window = m_ctx.windowsByXid[i].window;

        if (window && window->isPendingDeletion()) {
            delete window;
            // Swap with last element for O(1) removal, or shift elements down
            m_ctx.windowsByXid[i] = m_ctx.windowsByXid.back();
            m_ctx.windowsByXid.pop_back();
        } else {
            ++i;
        }
    }
}

void X11Backend::waitEvents() {
    waitForEventsX11(m_ctx, m_quitRequestCallback, m_displayChangeCallback);
}

void X11Backend::waitEventsTimeout(double timeoutSeconds) {
    waitForEventsWithTimeoutX11(m_ctx, timeoutSeconds, m_quitRequestCallback,
                                m_displayChangeCallback);
}

void X11Backend::setQuitRequestCallback(nostd::Function<bool()> callback) {
    m_quitRequestCallback = std::move(callback);
}
void X11Backend::setDisplayChangeCallback(nostd::Function<void()> callback) {
    m_displayChangeCallback = std::move(callback);
}
void X11Backend::setSystemThemeChangeCallback(
    nostd::Function<void(VeraSystemTheme)> callback) {
    setThemeChangeCallbackX11(std::move(callback));
}

nostd::Vector<VeraMonitorInfo> X11Backend::getMonitors() const {
    return getMonitorsX11(m_ctx);
}
VeraMonitorInfo X11Backend::getPrimaryMonitor() const {
    return getPrimaryMonitorX11(m_ctx);
}
VeraMonitorInfo X11Backend::getMonitorAt(int32_t x, int32_t y) const {
    return getMonitorAtCoordinateXYX11(m_ctx, x, y);
}
nostd::Vector<VeraDisplayModeInfo> X11Backend::getSupportedDisplayModes(
    const VeraMonitorInfo& monitor) const {
    return getSupportedDisplayModesX11(m_ctx, monitor);
}

bool X11Backend::supportsNativeDecorationHitTesting() const { return false; }

VeraStringView X11Backend::getClipboardText() const {
    return getClipboardTextX11(m_ctx);
}
void X11Backend::setClipboardText(const char* text) {
    setClipboardTextX11(m_ctx, text);
}
bool X11Backend::hasClipboardText() const { return hasClipboardTextX11(m_ctx); }

void X11Backend::setDragCallback(VeraDragCallback callback) {
    setCallback(std::move(callback));
}

VeraSystemTheme X11Backend::getSystemTheme() const {
    return getCurrentThemeX11(m_ctx);
}

nostd::Vector<VeraInputDeviceInfo> X11Backend::getInputDevices() const {
    if (!m_hasXInput2) return {};
    return enumerateInputDevicesX11(m_ctx);
}

VeraNativeHandle X11Backend::getNativeHandle() const {
    VeraNativeHandle handle;
    handle.display = m_ctx.display;
    return handle;
}

void X11Backend::applySettings(const VeraSettings& settings) {
    (void)settings;
    // m_ctx.keyRepeatDelay = settings.keyRepeatSettings.delayMs;
    // m_ctx.keyRepeatRate = settings.keyRepeatSettings.rate;
}

void X11Backend::setCursorShape(VeraCursorShape cursh) {
    applyCursorShapeX11(m_ctx, m_ctx.screen, cursh);
}

// Direct indexed lookup table (O(1) with zero hashing/allocation overhead)
static Cursor gShapeCache[static_cast<size_t>(VeraCursorShape::Count)] = {0};
static Cursor gBlankCursor = 0;

static unsigned int shapeToFontGlyph(VeraCursorShape shape) {
    switch (shape) {
        case VeraCursorShape::Arrow:
            return XC_left_ptr;
        case VeraCursorShape::IBeam:
            return XC_xterm;
        case VeraCursorShape::Crosshair:
            return XC_crosshair;
        case VeraCursorShape::Hand:
            return XC_hand2;
        case VeraCursorShape::HResize:
            return XC_sb_h_double_arrow;
        case VeraCursorShape::VResize:
            return XC_sb_v_double_arrow;
        case VeraCursorShape::CornerResizeNWSE:
            return XC_bottom_right_corner;
        case VeraCursorShape::CornerResizeNESW:
            return XC_bottom_left_corner;
        case VeraCursorShape::NotAllowed:
            return XC_X_cursor;
        default:
            return XC_left_ptr;
    }
}

static Cursor getOrCreateBlankCursor(X11Context& ctx, Window window) {
    if (gBlankCursor) return gBlankCursor;

    char data[1] = {0};
    Pixmap blankPixmap = XCreateBitmapFromData(ctx.display, window, data, 1, 1);
    XColor black{};
    gBlankCursor = XCreatePixmapCursor(ctx.display, blankPixmap, blankPixmap,
                                       &black, &black, 0, 0);
    XFreePixmap(ctx.display, blankPixmap);
    return gBlankCursor;
}

void applyCursorShapeX11(X11Context& ctx, Window window,
                         VeraCursorShape shape) {
    size_t idx = static_cast<size_t>(shape);
    if (idx >= static_cast<size_t>(VeraCursorShape::Count)) return;

    if (gShapeCache[idx] == 0) {
        gShapeCache[idx] =
            XCreateFontCursor(ctx.display, shapeToFontGlyph(shape));
    }

    XDefineCursor(ctx.display, window, gShapeCache[idx]);
}

void applyCursorModeX11(X11Context& ctx, Window window, VeraCursorMode mode) {
    switch (mode) {
        case VeraCursorMode::Normal:
            XUngrabPointer(ctx.display, CurrentTime);
            XUndefineCursor(ctx.display, window);
            break;
        case VeraCursorMode::Hidden:
            XUngrabPointer(ctx.display, CurrentTime);
            XDefineCursor(ctx.display, window,
                          getOrCreateBlankCursor(ctx, window));
            break;
        case VeraCursorMode::Disabled:
            XDefineCursor(ctx.display, window,
                          getOrCreateBlankCursor(ctx, window));
            XGrabPointer(
                ctx.display, window, True,
                ButtonPressMask | ButtonReleaseMask | PointerMotionMask,
                GrabModeAsync, GrabModeAsync, window, None, CurrentTime);
            break;
    }
}

void shutdownCursorX11(X11Context& ctx) {
    constexpr size_t count = static_cast<size_t>(VeraCursorShape::Count);
    for (size_t i = 0; i < count; ++i) {
        if (gShapeCache[i] != 0) {
            XFreeCursor(ctx.display, gShapeCache[i]);
            gShapeCache[i] = 0;
        }
    }

    if (gBlankCursor != 0) {
        XFreeCursor(ctx.display, gBlankCursor);
        gBlankCursor = 0;
    }
}

#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <unistd.h>

#include <string>

static std::string gOwnedText;
static bool gOwningSelection = false;

void initializeClipboardX11(X11Context& ctx) {
    ctx.clipboardOwnerWindow =
        XCreateSimpleWindow(ctx.display, ctx.root, -10, -10, 1, 1, 0, 0, 0);
}

void setClipboardTextX11(X11Context& ctx, const char* text) {
    gOwnedText = text;
    gOwningSelection = true;
    XSetSelectionOwner(ctx.display, ctx.atoms.clipboard,
                       ctx.clipboardOwnerWindow, CurrentTime);
}

VeraStringView getClipboardTextX11(X11Context& ctx) {
    Window owner = XGetSelectionOwner(ctx.display, ctx.atoms.clipboard);
    if (owner == None) return {};

    if (owner == ctx.clipboardOwnerWindow) {
        return {.data = gOwnedText.data(), .length = gOwnedText.length()};
    }

    Atom property = XInternAtom(ctx.display, "VERA_CLIPBOARD_TRANSFER", False);
    XConvertSelection(ctx.display, ctx.atoms.clipboard, ctx.atoms.utf8String,
                      property, ctx.clipboardOwnerWindow, CurrentTime);

    XEvent event;
    for (int attempts = 0; attempts < 200; ++attempts) {
        if (XCheckTypedWindowEvent(ctx.display, ctx.clipboardOwnerWindow,
                                   SelectionNotify, &event)) {
            break;
        }
        XFlush(ctx.display);
        usleep(1000);
    }
    if (event.type != SelectionNotify || event.xselection.property == None) {
        return {};
    }

    Atom actualType;
    int actualFormat;
    ulong itemCount, bytesAfter;
    unsigned char* data = nullptr;
    if (XGetWindowProperty(ctx.display, ctx.clipboardOwnerWindow, property, 0,
                           1 << 20, True, AnyPropertyType, &actualType,
                           &actualFormat, &itemCount, &bytesAfter,
                           &data) == Success &&
        data) {
        gOwnedText.assign(reinterpret_cast<char*>(data), itemCount);
        XFree(data);
    }
    return {.data = gOwnedText.data(), .length = gOwnedText.length()};
}

bool hasClipboardTextX11(X11Context& ctx) {
    return XGetSelectionOwner(ctx.display, ctx.atoms.clipboard) != None;
}

void handleClipboardSelectionRequestX11(X11Context& ctx,
                                        XSelectionRequestEvent& request) {
    XSelectionEvent response{};
    response.type = SelectionNotify;
    response.requestor = request.requestor;
    response.selection = request.selection;
    response.target = request.target;
    response.time = request.time;
    response.property = None;

    if (request.target == ctx.atoms.targets) {
        Atom targets[] = {ctx.atoms.targets, ctx.atoms.utf8String, XA_STRING};
        XChangeProperty(ctx.display, request.requestor, request.property,
                        XA_ATOM, 32, PropModeReplace,
                        reinterpret_cast<unsigned char*>(targets), 3);
        response.property = request.property;
    } else if (request.target == ctx.atoms.utf8String ||
               request.target == XA_STRING) {
        XChangeProperty(
            ctx.display, request.requestor, request.property, request.target, 8,
            PropModeReplace,
            reinterpret_cast<const unsigned char*>(gOwnedText.data()),
            static_cast<int>(gOwnedText.size()));
        response.property = request.property;
    }

    XSendEvent(ctx.display, request.requestor, False, NoEventMask,
               reinterpret_cast<XEvent*>(&response));
}

void handleClipboardSelectionClearX11(X11Context&, XSelectionClearEvent&) {
    gOwningSelection = false;
    gOwnedText.clear();
}

static VeraDragCallback gCallback;
static Window gSourceWindow = 0;
static int32_t gPendingX = 0, gPendingY = 0;

void setCallback(VeraDragCallback callback) { gCallback = std::move(callback); }

static nostd::Vector<const char*> parseUriList(const char* data, size_t len) {
    nostd::Vector<const char*> paths;
    if (!data || len == 0) return paths;

    const char* ptr = data;
    const char* end = data + len;

    while (ptr < end) {
        // Find line boundaries (\r\n or \n)
        const char* lineStart = ptr;
        while (ptr < end && *ptr != '\n' && *ptr != '\r') {
            ptr++;
        }

        size_t lineLen = ptr - lineStart;

        // Skip newline delimiters
        while (ptr < end && (*ptr == '\n' || *ptr == '\r')) {
            ptr++;
        }

        // Skip empty lines or comments
        if (lineLen == 0 || lineStart[0] == '#') continue;

        // Strip "file://" prefix if present
        constexpr const char kFilePrefix[] = "file://";
        constexpr size_t kPrefixLen = sizeof(kFilePrefix) - 1;

        if (lineLen >= kPrefixLen &&
            std::strncmp(lineStart, kFilePrefix, kPrefixLen) == 0) {
            lineStart += kPrefixLen;
            lineLen -= kPrefixLen;
        }

        if (lineLen > 0) {
            // Allocate exact buffer for null-terminated path
            char* pathBuf = new char[lineLen + 1];
            std::memcpy(pathBuf, lineStart, lineLen);
            pathBuf[lineLen] = '\0';

            paths.push_back(pathBuf);
        }
    }

    return paths;
}

void handleClientMessage(X11Context& ctx, VeraWindow* window,
                         XClientMessageEvent& event) {
    if (!gCallback) return;

    if (event.message_type == ctx.atoms.xdndEnter) {
        gSourceWindow = static_cast<Window>(event.data.l[0]);
        gCallback(VeraDragEvent{VeraDragAction::Enter, window, 0, 0, {}, 0});
    } else if (event.message_type == ctx.atoms.xdndPosition) {
        int32_t rootX = static_cast<int32_t>(event.data.l[2]) >> 16;
        int32_t rootY = static_cast<int32_t>(event.data.l[2]) & 0xFFFF;
        gPendingX = rootX;
        gPendingY = rootY;
        bool accept = gCallback(
            VeraDragEvent{VeraDragAction::Over, window, rootX, rootY, {}, 0});

        XClientMessageEvent status{};
        status.type = ClientMessage;
        status.window = event.data.l[0];
        status.message_type = ctx.atoms.xdndStatus;
        status.format = 32;
        status.data.l[0] =
            static_cast<int64_t>(window->getNativeHandle().x11Window);
        status.data.l[1] = accept ? 1 : 0;
        status.data.l[4] =
            accept ? static_cast<int64_t>(ctx.atoms.xdndActionCopy) : 0;
        XSendEvent(ctx.display, event.data.l[0], False, NoEventMask,
                   reinterpret_cast<XEvent*>(&status));
    } else if (event.message_type == ctx.atoms.xdndDrop) {
        Window target =
            static_cast<Window>(window->getNativeHandle().x11Window);
        XConvertSelection(ctx.display, ctx.atoms.xdndSelection,
                          ctx.atoms.textUriList, ctx.atoms.xdndSelection,
                          target, event.data.l[2] /* timestamp */);
    } else if (event.message_type == ctx.atoms.xdndLeave) {
        gCallback(VeraDragEvent{VeraDragAction::Leave, window, 0, 0, {}, 0});
    }
}

void handleSelectionNotify(X11Context& ctx, VeraWindow* window,
                           XSelectionEvent& event) {
    if (event.property == None || !gCallback) return;

    Atom actualType;
    int actualFormat;
    ulong itemCount, bytesAfter;
    unsigned char* data = nullptr;
    Window target = static_cast<Window>(window->getNativeHandle().x11Window);

    if (XGetWindowProperty(ctx.display, target, event.property, 0, 1 << 20,
                           True, AnyPropertyType, &actualType, &actualFormat,
                           &itemCount, &bytesAfter, &data) == Success &&
        data) {
        // Parse uriList directly from raw byte buffer
        nostd::Vector<const char*> localPaths =
            parseUriList(reinterpret_cast<const char*>(data), itemCount);
        XFree(data);

        nostd::Vector<VeraStringView> paths;
        paths.reserve(localPaths.size());
        for (size_t i = 0; i < localPaths.size(); ++i) {
            const char* path = localPaths[i];
            paths.push_back({.data = path, .length = std::strlen(path)});
        }

        VeraDragEvent dropEvent{
            VeraDragAction::Drop,
            window,
            gPendingX,
            gPendingY,
            paths.data(),
            static_cast<uint32_t>(paths.size()),
        };
        gCallback(dropEvent);

        // Clean up heap string allocations from parseUriList
        for (size_t i = 0; i < localPaths.size(); ++i) {
            delete[] localPaths[i];
        }
    }

    XClientMessageEvent finished{};
    finished.type = ClientMessage;
    finished.window = gSourceWindow;
    finished.message_type = ctx.atoms.xdndFinished;
    finished.format = 32;
    finished.data.l[0] = static_cast<int64_t>(target);
    finished.data.l[1] = 1;
    finished.data.l[2] = static_cast<int64_t>(ctx.atoms.xdndActionCopy);
    XSendEvent(ctx.display, gSourceWindow, False, NoEventMask,
               reinterpret_cast<XEvent*>(&finished));
}

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <cstring>

static nostd::Function<void(VeraSystemTheme)> gThemeCallback;
static Window gSettingsOwner = 0;

void initializeThemeX11(X11Context& ctx) {
    std::string selectionName = "_XSETTINGS_S" + std::to_string(ctx.screen);
    Atom selectionAtom = XInternAtom(ctx.display, selectionName.c_str(), False);
    gSettingsOwner = XGetSelectionOwner(ctx.display, selectionAtom);
    if (gSettingsOwner != None) {
        XSelectInput(ctx.display, gSettingsOwner, PropertyChangeMask);
    }
}

static bool findBoolSetting(const unsigned char* data, ulong length,
                            const char* settingName, bool& outValue) {
    const char* haystack = reinterpret_cast<const char*>(data);
    const char* found = static_cast<const char*>(
        memmem(haystack, length, settingName, strlen(settingName)));
    if (!found) return false;

    outValue = true;
    return true;
}

VeraSystemTheme getCurrentThemeX11(X11Context& ctx) {
    if (gSettingsOwner == None) return VeraSystemTheme::Unknown;

    Atom actualType;
    int actualFormat;
    ulong itemCount, bytesAfter;
    unsigned char* data = nullptr;

    Atom settingsAtom = ctx.atoms.xSettingsSettings;
    if (XGetWindowProperty(ctx.display, gSettingsOwner, settingsAtom, 0,
                           1 << 16, False, AnyPropertyType, &actualType,
                           &actualFormat, &itemCount, &bytesAfter,
                           &data) != Success ||
        !data) {
        return VeraSystemTheme::Unknown;
    }

    VeraSystemTheme theme = VeraSystemTheme::Unknown;
    bool preferDark = false;
    if (findBoolSetting(data, itemCount, "Gtk/ApplicationPreferDarkTheme",
                        preferDark)) {
        theme = preferDark ? VeraSystemTheme::Dark : VeraSystemTheme::Light;
    }
    XFree(data);
    return theme;
}

void setThemeChangeCallbackX11(
    nostd::Function<void(VeraSystemTheme)> callback) {
    gThemeCallback = std::move(callback);
}

void handleThemePropertyNotifyX11(X11Context& ctx, XPropertyEvent& event) {
    if (event.window != gSettingsOwner ||
        event.atom != ctx.atoms.xSettingsSettings) {
        return;
    }
    if (gThemeCallback) gThemeCallback(getCurrentThemeX11(ctx));
}

#include <X11/Xlib.h>
#include <X11/extensions/Xrandr.h>
#include <sys/select.h>
#include <unistd.h>

#include <cstdint>

static int gRandrEventBase = 0;

static void dispatchOne(X11Context& ctx, XEvent& event,
                        const nostd::Function<bool()>& quitRequestCallback,
                        const nostd::Function<void()>& displayChangeCallback) {
    if (gRandrEventBase &&
        event.type == gRandrEventBase + RRScreenChangeNotify) {
        XRRUpdateConfiguration(&event);
        if (displayChangeCallback) displayChangeCallback();
        return;
    }

    switch (event.type) {
        case ClientMessage: {
            X11Window* window = findWindowByXid(ctx, event.xclient.window);
            if (!window) return;

            if (event.xclient.message_type == ctx.atoms.wmProtocols &&
                static_cast<Atom>(event.xclient.data.l[0]) ==
                    ctx.atoms.wmDeleteWindow) {
                bool allowQuit = true;
                if (quitRequestCallback) {
                    allowQuit = quitRequestCallback();
                }

                if (allowQuit) {
                    window->handleWmCloseRequest();
                }
                return;
            }
            handleClientMessage(ctx, window, event.xclient);
            return;
        }
        case SelectionRequest:
            handleClipboardSelectionRequestX11(ctx, event.xselectionrequest);
            return;
        case SelectionClear:
            handleClipboardSelectionClearX11(ctx, event.xselectionclear);
            return;
        case SelectionNotify: {
            for (size_t i = 0; i < ctx.windowsByXid.size(); ++i) {
                X11Window* window = ctx.windowsByXid[i].window;
                if (window &&
                    static_cast<Window>(window->getNativeHandle().x11Window) ==
                        event.xselection.requestor) {
                    handleSelectionNotify(ctx, window, event.xselection);
                    break;
                }
            }
            return;
        }
        case PropertyNotify:
            handleThemePropertyNotifyX11(ctx, event.xproperty);
            [[fallthrough]];
        default: {
            X11Window* window = findWindowByXid(ctx, event.xany.window);
            if (window) window->handleXEvent(event);
        }
    }
}

uint64_t getMonotonicMs() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return static_cast<uint64_t>(ts.tv_sec) * 1000 +
           static_cast<uint64_t>(ts.tv_nsec / 1000000);
}

static void processKeyRepeat(X11Context* ctx) {
    if (!ctx || ctx->keyRepeatRate == 0 || ctx->pressedKeys.empty()) {
        return;
    }

    uint64_t nowMs = getMonotonicMs();

    for (size_t i = 0; i < ctx->pressedKeys.size(); ++i) {
        KeyRepeatStateX11& repeatState = ctx->pressedKeys[i];

        if (nowMs >= repeatState.nextRepeatMs) {
            X11Window* window = findWindowByXid(*ctx, repeatState.window);
            if (window && window->getKeyCallback()) {
                window->getKeyCallback()(repeatState.veraKey, true, true);
            }

            repeatState.nextRepeatMs = nowMs + (1000 / ctx->keyRepeatRate);
        }
    }
}

void pollEventsX11(X11Context& ctx,
                   const nostd::Function<bool()>& quitRequestCallback,
                   const nostd::Function<void()>& displayChangeCallback) {
    if (!gRandrEventBase) {
        int errorBase;
        XRRQueryExtension(ctx.display, &gRandrEventBase, &errorBase);
    }

    // Read non-blocking input updates from controller drivers
    updateJoystickX11(ctx);

    while (XPending(ctx.display) > 0) {
        XEvent event;
        XNextEvent(ctx.display, &event);
        dispatchOne(ctx, event, quitRequestCallback, displayChangeCallback);
    }

    processKeyRepeat(&ctx);
}

void waitForEventsX11(X11Context& ctx,
                      const nostd::Function<bool()>& quitRequestCallback,
                      const nostd::Function<void()>& displayChangeCallback) {
    if (XPending(ctx.display) > 0) {
        pollEventsX11(ctx, quitRequestCallback, displayChangeCallback);
        return;
    }

    // Direct fallback into our multi-device select handler using a safe 10ms
    // responsive step
    waitForEventsWithTimeoutX11(ctx, 0.010, quitRequestCallback,
                                displayChangeCallback);
}

void waitForEventsWithTimeoutX11(
    X11Context& ctx, double timeoutSeconds,
    const nostd::Function<bool()>& quitRequestCallback,
    const nostd::Function<void()>& displayChangeCallback) {
    // Refresh joystick state maps before evaluating thread blocks
    updateJoystickX11(ctx);

    if (XPending(ctx.display) > 0) {
        pollEventsX11(ctx, quitRequestCallback, displayChangeCallback);
        return;
    }

    int x11Fd = ConnectionNumber(ctx.display);
    int maxFd = x11Fd;

    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(x11Fd, &fds);

    // Multiplex all active context joystick file descriptors directly into
    // select
    for (const auto& joy : ctx.joysticks) {
        if (joy.fd >= 0) {
            FD_SET(joy.fd, &fds);
            if (joy.fd > maxFd) {
                maxFd = joy.fd;
            }
        }
    }

    timeval tv;
    tv.tv_sec = static_cast<int64_t>(timeoutSeconds);
    tv.tv_usec = static_cast<int64_t>((timeoutSeconds - tv.tv_sec) * 1'000'000);

    // Thread wakes immediately when X11 messages arrive OR joystick events fire
    if (select(maxFd + 1, &fds, nullptr, nullptr, &tv) > 0) {
        pollEventsX11(ctx, quitRequestCallback, displayChangeCallback);
    } else {
        // Fallback update to process joystick disconnection rules and edge
        // clocks on timeouts
        updateJoystickX11(ctx);
        processKeyRepeat(&ctx);
    }
}

#include <dirent.h>
#include <fcntl.h>
#include <linux/joystick.h>
#include <sys/ioctl.h>
#include <unistd.h>

static nostd::Function<void(uint32_t, uint32_t, bool)> gButtonCallback;
static nostd::Function<void(uint32_t, uint32_t, float)> gAxisCallback;

void setJoystickButtonCallbackX11(
    nostd::Function<void(uint32_t, uint32_t, bool)> cb) {
    gButtonCallback = cb;
}
void setJoystickAxisCallbackX11(
    nostd::Function<void(uint32_t, uint32_t, float)> cb) {
    gAxisCallback = cb;
}

void initializeJoystickX11(X11Context& ctx) {
    DIR* devDir = opendir("/dev/input");
    if (!devDir) {
        printf("[Joystick] Failed to open /dev/input directory.\n");
        return;
    }

    struct dirent* entry;
    uint32_t slot = 0;

    while ((entry = readdir(devDir)) != nullptr &&
           slot < ctx.joysticks.size()) {
        // Fast prefix match for "js*"
        if (strncmp(entry->d_name, "js", 2) == 0) {
            char path[MAX_JOYSTICK_PATH] = "/dev/input/";
            constexpr size_t prefixLen = 11;  // strlen("/dev/input/")

            // Append device filename directly onto path buffer
            strncpy(path + prefixLen, entry->d_name,
                    sizeof(path) - prefixLen - 1);
            path[sizeof(path) - 1] = '\0';

            int fd = open(path, O_RDONLY | O_NONBLOCK);
            if (fd >= 0) {
                char devName[128] = "Unknown Controller";
                ioctl(fd, JSIOCGNAME(sizeof(devName)), devName);

                uint8_t axesCount = 0;
                uint8_t buttonsCount = 0;
                ioctl(fd, JSIOCGAXES, &axesCount);
                ioctl(fd, JSIOCGBUTTONS, &buttonsCount);

                JoystickDeviceX11& joy = ctx.joysticks[slot];
                joy.fd = fd;

                // Safe array copies without format parsing overhead
                strncpy(joy.devicePath, path, sizeof(joy.devicePath) - 1);
                joy.devicePath[sizeof(joy.devicePath) - 1] = '\0';

                joy.state.connected = true;

                strncpy(joy.state.name, devName, sizeof(joy.state.name) - 1);
                joy.state.name[sizeof(joy.state.name) - 1] = '\0';

                joy.state.axes.resize(axesCount, 0.0f);
                joy.state.buttons.resize(buttonsCount, false);

                printf("[Joystick] Registered [%u]: %s\n", slot,
                       joy.state.name);
                slot++;
            }
        }
    }
    closedir(devDir);
}

void updateJoystickX11(X11Context& ctx) {
    (void)ctx;

    for (size_t i = 0; i < ctx.joysticks.size(); ++i) {
        JoystickDeviceX11& joy = ctx.joysticks[i];
        if (joy.fd < 0) continue;

        struct js_event event;
        while (read(joy.fd, &event, sizeof(event)) > 0) {
            uint8_t type = event.type & ~JS_EVENT_INIT;

            if (type == JS_EVENT_BUTTON) {
                if (event.number < joy.state.buttons.size()) {
                    joy.state.buttons[event.number] = (event.value != 0);
                    if (gButtonCallback) {
                        gButtonCallback(static_cast<uint32_t>(i), event.number,
                                        joy.state.buttons[event.number]);
                    }
                }
            } else if (type == JS_EVENT_AXIS) {
                if (event.number < joy.state.axes.size()) {
                    joy.state.axes[event.number] =
                        static_cast<float>(event.value) / 32767.0f;
                    if (gAxisCallback) {
                        gAxisCallback(static_cast<uint32_t>(i), event.number,
                                      joy.state.axes[event.number]);
                    }
                }
            }
        }

        if (errno == ENODEV) {
            printf("[Joystick] Disconnected: %s", joy.state.name);
            close(joy.fd);
            joy.fd = -1;
            joy.state.connected = false;
        }
    }
}

VeraJoystickState getJoystickStateX11(X11Context& ctx, uint32_t joystickId) {
    if (joystickId < ctx.joysticks.size()) {
        return ctx.joysticks[joystickId].state;
    }
    return VeraJoystickState{};
}

void shutdownJoystickX11(X11Context& ctx) {
    (void)ctx;
    for (auto& joy : ctx.joysticks) {
        if (joy.fd >= 0) close(joy.fd);
        joy.fd = -1;
        joy.state.connected = false;
    }
}

size_t utf32ToUtf8(uint32_t cp, char out[5]) {
    if (cp <= 0x7F) {
        out[0] = static_cast<char>(cp);
        out[1] = '\0';
        return 1;
    } else if (cp <= 0x7FF) {
        out[0] = static_cast<char>(0xC0 | (cp >> 6));
        out[1] = static_cast<char>(0x80 | (cp & 0x3F));
        out[2] = '\0';
        return 2;
    } else if (cp <= 0xFFFF) {
        out[0] = static_cast<char>(0xE0 | (cp >> 12));
        out[1] = static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
        out[2] = static_cast<char>(0x80 | (cp & 0x3F));
        out[3] = '\0';
        return 3;
    } else if (cp <= 0x10FFFF) {
        out[0] = static_cast<char>(0xF0 | (cp >> 18));
        out[1] = static_cast<char>(0x80 | ((cp >> 12) & 0x3F));
        out[2] = static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
        out[3] = static_cast<char>(0x80 | (cp & 0x3F));
        out[4] = '\0';
        return 4;
    }
    out[0] = '\0';
    return 0;
}

void handleKeyPressX11(
    X11Context& ctx, XKeyEvent& event, KeyStateArray& state,
    const nostd::Function<void(VeraKey, bool, bool)>& keyCallback,
    const nostd::Function<void(const char*)>& charCallback) {
    VeraKey key = convertKeyEventToVeraKeyX11(ctx, event);

    size_t idx = static_cast<size_t>(key);
    bool repeat = false;

    if (key != VeraKey::Unknown && idx < state.size()) {
        repeat = state[idx];

        if (!repeat) {
            state[idx] = true;

            KeyRepeatStateX11 repeatState{};
            repeatState.window = event.window;
            repeatState.key = event.keycode;
            repeatState.scanCode = event.keycode;
            repeatState.veraKey = key;
            repeatState.nextRepeatMs = getMonotonicMs() + ctx.keyRepeatDelay;

            // Find existing entry or append to nostd::Vector
            bool found = false;
            for (size_t i = 0; i < ctx.pressedKeys.size(); ++i) {
                if (ctx.pressedKeys[i].key == event.keycode) {
                    ctx.pressedKeys[i] = repeatState;
                    found = true;
                    break;
                }
            }
            if (!found) {
                ctx.pressedKeys.push_back(repeatState);
            }
        }
    }

    // Ignore X11 auto-repeat events. Vera generates repeat events itself.
    if (repeat) {
        return;
    }

    if (keyCallback) {
        keyCallback(key, /*pressed=*/true, /*repeat=*/false);
    }

    if (charCallback) {
        uint32_t codepoint = convertKeyEventToCodepointX11(ctx, event);
        if (codepoint != 0) {
            char utf8Buf[5];
            if (utf32ToUtf8(codepoint, utf8Buf) > 0) {
                charCallback(utf8Buf);
            }
        }
    }
}

void handleKeyReleaseX11(
    X11Context& ctx, XKeyEvent& event, KeyStateArray& state,
    const nostd::Function<void(VeraKey, bool, bool)>& keyCallback) {
    VeraKey key = convertKeyEventToVeraKeyX11(ctx, event);

    // Linear search in nostd::Vector instead of map.find
    int foundIdx = -1;
    for (size_t i = 0; i < ctx.pressedKeys.size(); ++i) {
        if (ctx.pressedKeys[i].key == event.keycode) {
            foundIdx = static_cast<int>(i);
            break;
        }
    }

    if (foundIdx != -1) {
        VeraKey originalKey = ctx.pressedKeys[foundIdx].veraKey;
        if (originalKey != key) {
            size_t origIdx = static_cast<size_t>(originalKey);
            if (origIdx < state.size()) {
                state[origIdx] = false;
            }
            key = originalKey;  // Align final callback signatures with original
                                // press
        }

        // Fast O(1) swap-and-pop removal from vector
        ctx.pressedKeys[foundIdx] = ctx.pressedKeys.back();
        ctx.pressedKeys.resize(ctx.pressedKeys.size() - 1);
    } else {
        // Fallback for untracked corner cases
        size_t idx = static_cast<size_t>(key);
        if (key != VeraKey::Unknown && idx < state.size()) {
            state[idx] = false;
        }
    }

    if (keyCallback) {
        keyCallback(key, /*pressed=*/false, /*repeat=*/false);
    }
}

static bool mapButton(unsigned int xButton, VeraMouseButton& out) {
    switch (xButton) {
        case Button1:
            out = VeraMouseButton::Left;
            return true;
        case Button2:
            out = VeraMouseButton::Middle;
            return true;
        case Button3:
            out = VeraMouseButton::Right;
            return true;
        case 8:
            out = VeraMouseButton::VeraButton4;
            return true;
        case 9:
            out = VeraMouseButton::VeraButton5;
            return true;
        default:
            return false;
    }
}

void handleMouseButtonPressX11(
    XButtonEvent& event,
    const nostd::Function<void(VeraMouseButton, bool)>& buttonCallback,
    const nostd::Function<void(double, double)>& scrollCallback) {
    switch (event.button) {
        case Button4:
            if (scrollCallback) scrollCallback(0.0, 1.0);
            return;
        case Button5:
            if (scrollCallback) scrollCallback(0.0, -1.0);
            return;
        case 6:
            if (scrollCallback) scrollCallback(-1.0, 0.0);
            return;
        case 7:
            if (scrollCallback) scrollCallback(1.0, 0.0);
            return;
        default:
            break;
    }
    VeraMouseButton button;
    if (mapButton(event.button, button) && buttonCallback) {
        buttonCallback(button, true);
    }
}

void handleMouseButtonReleaseX11(
    XButtonEvent& event,
    const nostd::Function<void(VeraMouseButton, bool)>& buttonCallback) {
    VeraMouseButton button;
    if (mapButton(event.button, button) && buttonCallback) {
        buttonCallback(button, false);
    }
}

static Atom intern(Display* d, const char* name) {
    return XInternAtom(d, name, False);
}

void internAtomsX11(X11Context& ctx) {
    Display* d = ctx.display;
    X11Atoms& a = ctx.atoms;

    a.wmProtocols = intern(d, "WM_PROTOCOLS");
    a.wmDeleteWindow = intern(d, "WM_DELETE_WINDOW");
    a.wmState = intern(d, "WM_STATE");

    a.netWmState = intern(d, "_NET_WM_STATE");
    a.netWmStateFullscreen = intern(d, "_NET_WM_STATE_FULLSCREEN");
    a.netWmStateMaximizedHorz = intern(d, "_NET_WM_STATE_MAXIMIZED_HORZ");
    a.netWmStateMaximizedVert = intern(d, "_NET_WM_STATE_MAXIMIZED_VERT");
    a.netWmStateAbove = intern(d, "_NET_WM_STATE_ABOVE");
    a.netWmStateHidden = intern(d, "_NET_WM_STATE_HIDDEN");
    a.netWmName = intern(d, "_NET_WM_NAME");
    a.netWmIcon = intern(d, "_NET_WM_ICON");
    a.netWmIconName = intern(d, "_NET_WM_ICON_NAME");
    a.netWmWindowType = intern(d, "_NET_WM_WINDOW_TYPE");
    a.netWmWindowTypeNormal = intern(d, "_NET_WM_WINDOW_TYPE_NORMAL");
    a.netWmPid = intern(d, "_NET_WM_PID");
    a.netWmMoveresize = intern(d, "_NET_WM_MOVERESIZE");
    a.netWorkarea = intern(d, "_NET_WORKAREA");
    a.netCurrentDesktop = intern(d, "_NET_CURRENT_DESKTOP");
    a.netWmSyncRequest = intern(d, "_NET_WM_SYNC_REQUEST");

    a.motifWmHints = intern(d, "_MOTIF_WM_HINTS");

    a.utf8String = intern(d, "UTF8_STRING");
    a.clipboard = intern(d, "CLIPBOARD");
    a.targets = intern(d, "TARGETS");
    a.multiple = intern(d, "MULTIPLE");
    a.incr = intern(d, "INCR");

    a.xdndAware = intern(d, "XdndAware");
    a.xdndEnter = intern(d, "XdndEnter");
    a.xdndPosition = intern(d, "XdndPosition");
    a.xdndStatus = intern(d, "XdndStatus");
    a.xdndLeave = intern(d, "XdndLeave");
    a.xdndDrop = intern(d, "XdndDrop");
    a.xdndFinished = intern(d, "XdndFinished");
    a.xdndSelection = intern(d, "XdndSelection");
    a.xdndActionCopy = intern(d, "XdndActionCopy");
    a.textUriList = intern(d, "text/uri-list");

    a.xSettingsSettings = intern(d, "_XSETTINGS_SETTINGS");
}

#include <X11/XF86keysym.h>
#include <X11/XKBlib.h>
#include <X11/Xlib.h>

#include <array>
#include <cstdint>

void initializeXKBX11(X11Context& ctx) {
    int major = XkbMajorVersion, minor = XkbMinorVersion;
    XkbLibraryVersion(&major, &minor);
    XkbQueryExtension(ctx.display, nullptr, nullptr, nullptr, &major, &minor);

    Bool supported = False;
    XkbSetDetectableAutoRepeat(ctx.display, True, &supported);
}

#include <linux/input-event-codes.h>

static VeraKey translateScancode(uint32_t scancode) {
    switch (scancode) {
        // Letters (Physical Keys A-Z)
        case KEY_A:
            return VeraKey::A;
        case KEY_B:
            return VeraKey::B;
        case KEY_C:
            return VeraKey::C;
        case KEY_D:
            return VeraKey::D;
        case KEY_E:
            return VeraKey::E;
        case KEY_F:
            return VeraKey::F;
        case KEY_G:
            return VeraKey::G;
        case KEY_H:
            return VeraKey::H;
        case KEY_I:
            return VeraKey::I;
        case KEY_J:
            return VeraKey::J;
        case KEY_K:
            return VeraKey::K;
        case KEY_L:
            return VeraKey::L;
        case KEY_M:
            return VeraKey::M;
        case KEY_N:
            return VeraKey::N;
        case KEY_O:
            return VeraKey::O;
        case KEY_P:
            return VeraKey::P;
        case KEY_Q:
            return VeraKey::Q;
        case KEY_R:
            return VeraKey::R;
        case KEY_S:
            return VeraKey::S;
        case KEY_T:
            return VeraKey::T;
        case KEY_U:
            return VeraKey::U;
        case KEY_V:
            return VeraKey::V;
        case KEY_W:
            return VeraKey::W;
        case KEY_X:
            return VeraKey::X;
        case KEY_Y:
            return VeraKey::Y;
        case KEY_Z:
            return VeraKey::Z;

        // Number Row (Base Physical Keys)
        case KEY_0:
            return VeraKey::Num0;
        case KEY_1:
            return VeraKey::Num1;
        case KEY_2:
            return VeraKey::Num2;
        case KEY_3:
            return VeraKey::Num3;
        case KEY_4:
            return VeraKey::Num4;
        case KEY_5:
            return VeraKey::Num5;
        case KEY_6:
            return VeraKey::Num6;
        case KEY_7:
            return VeraKey::Num7;
        case KEY_8:
            return VeraKey::Num8;
        case KEY_9:
            return VeraKey::Num9;

        // Base Symbols & Punctuation
        case KEY_SPACE:
            return VeraKey::Space;
        case KEY_APOSTROPHE:
            return VeraKey::Apostrophe;
        case KEY_COMMA:
            return VeraKey::Comma;
        case KEY_MINUS:
            return VeraKey::Minus;
        case KEY_DOT:
            return VeraKey::Period;
        case KEY_SLASH:
            return VeraKey::Slash;
        case KEY_SEMICOLON:
            return VeraKey::Semicolon;
        case KEY_EQUAL:
            return VeraKey::Equal;
        case KEY_LEFTBRACE:
            return VeraKey::LeftBracket;
        case KEY_BACKSLASH:
            return VeraKey::Backslash;
        case KEY_RIGHTBRACE:
            return VeraKey::RightBracket;
        case KEY_GRAVE:
            return VeraKey::GraveAccent;

        // System & Navigation
        case KEY_ENTER:
            return VeraKey::Enter;
        case KEY_ESC:
            return VeraKey::Escape;
        case KEY_TAB:
            return VeraKey::Tab;
        case KEY_BACKSPACE:
            return VeraKey::Backspace;
        case KEY_INSERT:
            return VeraKey::Insert;
        case KEY_DELETE:
            return VeraKey::Delete;
        case KEY_HOME:
            return VeraKey::Home;
        case KEY_END:
            return VeraKey::End;
        case KEY_PAGEUP:
            return VeraKey::PageUp;
        case KEY_PAGEDOWN:
            return VeraKey::PageDown;
        case KEY_LEFT:
            return VeraKey::Left;
        case KEY_RIGHT:
            return VeraKey::Right;
        case KEY_UP:
            return VeraKey::Up;
        case KEY_DOWN:
            return VeraKey::Down;

        // Modifiers
        case KEY_LEFTSHIFT:
            return VeraKey::LeftShift;
        case KEY_RIGHTSHIFT:
            return VeraKey::RightShift;
        case KEY_LEFTCTRL:
            return VeraKey::LeftCtrl;
        case KEY_RIGHTCTRL:
            return VeraKey::RightCtrl;
        case KEY_LEFTALT:
            return VeraKey::LeftAlt;
        case KEY_RIGHTALT:
            return VeraKey::RightAlt;
        case KEY_LEFTMETA:
            return VeraKey::LeftSuper;
        case KEY_RIGHTMETA:
            return VeraKey::RightSuper;

        // Locks & System Utilities
        case KEY_CAPSLOCK:
            return VeraKey::CapsLock;
        case KEY_SCROLLLOCK:
            return VeraKey::ScrollLock;
        case KEY_NUMLOCK:
            return VeraKey::NumLock;
        case KEY_PRINT:
            return VeraKey::PrintScreen;
        case KEY_PAUSE:
            return VeraKey::Pause;
        case KEY_COMPOSE:
            return VeraKey::Menu;

        // Function Keys
        case KEY_F1:
            return VeraKey::F1;
        case KEY_F2:
            return VeraKey::F2;
        case KEY_F3:
            return VeraKey::F3;
        case KEY_F4:
            return VeraKey::F4;
        case KEY_F5:
            return VeraKey::F5;
        case KEY_F6:
            return VeraKey::F6;
        case KEY_F7:
            return VeraKey::F7;
        case KEY_F8:
            return VeraKey::F8;
        case KEY_F9:
            return VeraKey::F9;
        case KEY_F10:
            return VeraKey::F10;
        case KEY_F11:
            return VeraKey::F11;
        case KEY_F12:
            return VeraKey::F12;

        // Keypad
        case KEY_KP0:
            return VeraKey::KP0;
        case KEY_KP1:
            return VeraKey::KP1;
        case KEY_KP2:
            return VeraKey::KP2;
        case KEY_KP3:
            return VeraKey::KP3;
        case KEY_KP4:
            return VeraKey::KP4;
        case KEY_KP5:
            return VeraKey::KP5;
        case KEY_KP6:
            return VeraKey::KP6;
        case KEY_KP7:
            return VeraKey::KP7;
        case KEY_KP8:
            return VeraKey::KP8;
        case KEY_KP9:
            return VeraKey::KP9;
        case KEY_KPDOT:
            return VeraKey::KPDecimal;
        case KEY_KPSLASH:
            return VeraKey::KPDivide;
        case KEY_KPASTERISK:
            return VeraKey::KPMultiply;
        case KEY_KPMINUS:
            return VeraKey::KPSubtract;
        case KEY_KPPLUS:
            return VeraKey::KPAdd;
        case KEY_KPENTER:
            return VeraKey::KPEnter;
        case KEY_KPEQUAL:
            return VeraKey::KPEqual;

        // Media Controls
        case KEY_MUTE:
            return VeraKey::Mute;
        case KEY_VOLUMEUP:
            return VeraKey::VolumeUp;
        case KEY_VOLUMEDOWN:
            return VeraKey::VolumeDown;

        default:
            return VeraKey::Unknown;
    }
}

// VeraKey convertKeyEventToVeraKeyX11(X11Context& ctx, XKeyEvent& event) {
//     KeySym ks = NoSymbol;
//
//     // We use column index matching based on Shift / CapsLock status
//     int col = 0;
//     if (event.state & (ShiftMask | LockMask)) {
//         col = 1;
//     }
//
//     // Try to get the active layout keysym with context modifiers
//     ks = XkbKeycodeToKeysym(ctx.display, event.keycode, 0, col);
//
//     // Fallback logic if column 1 yields null (like on Function keys or
//     Control
//     // maps)
//     if (ks == NoSymbol) {
//         ks = XkbKeycodeToKeysym(ctx.display, event.keycode, 0, 0);
//     }
//
//     return translateScancode(ks);
// }

VeraKey convertKeyEventToVeraKeyX11(X11Context& ctx, XKeyEvent& event) {
    (void)ctx;
    // X11 keycodes are offset by +8 from Linux evdev scancodes
    uint32_t scancode = (event.keycode >= 8) ? (event.keycode - 8) : 0;
    return translateScancode(scancode);
}

uint32_t convertKeyEventToCodepointX11(X11Context& ctx, XKeyEvent& event) {
    char buffer[32]{};
    KeySym keysym = NoSymbol;
    Status status;

    int len = 0;
    XIC targetXic = nullptr;

    X11Window* window = findWindowByXid(ctx, event.window);
    if (window) {
        targetXic = window->getXIC();
    }

    if (targetXic) {
        len = Xutf8LookupString(targetXic, &event, buffer,
                                static_cast<int>(sizeof(buffer)), &keysym,
                                &status);
    } else {
        len = XLookupString(&event, buffer, static_cast<int>(sizeof(buffer)),
                            &keysym, nullptr);
    }

    if (len <= 0) return 0;

    uint32_t codepoint = 0;
    auto u8Lead = static_cast<unsigned char>(buffer[0]);

    if (u8Lead < 0x80) {
        codepoint = u8Lead;
    } else if ((u8Lead & 0xE0) == 0xC0 && len >= 2) {
        codepoint = ((u8Lead & 0x1F) << 6) |
                    (static_cast<unsigned char>(buffer[1]) & 0x3F);
    } else if ((u8Lead & 0xF0) == 0xE0 && len >= 3) {
        codepoint = ((u8Lead & 0x0F) << 12) |
                    ((static_cast<unsigned char>(buffer[1]) & 0x3F) << 6) |
                    (static_cast<unsigned char>(buffer[2]) & 0x3F);
    } else if ((u8Lead & 0xF8) == 0xF0 && len >= 4) {
        codepoint = ((u8Lead & 0x07) << 18) |
                    ((static_cast<unsigned char>(buffer[1]) & 0x3F) << 12) |
                    ((static_cast<unsigned char>(buffer[2]) & 0x3F) << 6) |
                    (static_cast<unsigned char>(buffer[3]) & 0x3F);
    }

    return codepoint;
}

static bool gHasRandR = false;

bool initializeMonitorX11(X11Context& ctx) {
    gHasRandR = initializeXRandRX11(ctx);
    return true;
}

static VeraMonitorInfo fallbackWholeScreen(X11Context& ctx) {
    VeraMonitorInfo info;
    info.name = "X11-Screen-0";
    info.x = 0;
    info.y = 0;
    info.workAreaX = 0;
    info.workAreaY = 0;
    info.workAreaWidth =
        static_cast<uint32_t>(DisplayWidth(ctx.display, ctx.screen));
    info.workAreaHeight =
        static_cast<uint32_t>(DisplayHeight(ctx.display, ctx.screen));
    info.dpiScale = 1.0f;
    info.refreshRateHz = 60;
    info.isPrimary = true;
    return info;
}

nostd::Vector<VeraMonitorInfo> getMonitorsX11(X11Context& ctx) {
    if (gHasRandR) {
        auto monitors = queryMonitorsX11(ctx);
        if (!monitors.empty()) return monitors;
    }
    nostd::Vector<VeraMonitorInfo> list;
    list.push_back(fallbackWholeScreen(ctx));
    return list;
}

VeraMonitorInfo getPrimaryMonitorX11(X11Context& ctx) {
    for (auto& m : getMonitorsX11(ctx)) {
        if (m.isPrimary) return m;
    }
    auto monitors = getMonitorsX11(ctx);
    return monitors.empty() ? fallbackWholeScreen(ctx) : monitors.front();
}

VeraMonitorInfo getMonitorAtCoordinateXYX11(X11Context& ctx, int32_t x,
                                            int32_t y) {
    for (auto& m : getMonitorsX11(ctx)) {
        bool inX = x >= m.x && x < m.x + static_cast<int32_t>(m.workAreaWidth);
        bool inY = y >= m.y && y < m.y + static_cast<int32_t>(m.workAreaHeight);
        if (inX && inY) return m;
    }
    return getPrimaryMonitorX11(ctx);
}

nostd::Vector<VeraDisplayModeInfo> getSupportedDisplayModesX11(
    X11Context& ctx, const VeraMonitorInfo& monitor) {
    if (!gHasRandR) return {};
    return queryDisplayModesX11(ctx, monitor);
}

#include <X11/Xresource.h>
#include <X11/extensions/Xrandr.h>

#include <cmath>
#include <cstdlib>

bool initializeXRandRX11(X11Context& ctx) {
    int major, minor;
    if (!XRRQueryVersion(ctx.display, &major, &minor)) return false;
    return (major > 1) || (major == 1 && minor >= 5);
}

float queryDpiScaleX11(X11Context& ctx) {
    XrmInitialize();
    char* resourceString = XResourceManagerString(ctx.display);
    if (!resourceString) return 1.0f;

    XrmDatabase db = XrmGetStringDatabase(resourceString);
    if (!db) return 1.0f;

    char* type = nullptr;
    XrmValue value;
    float scale = 1.0f;
    if (XrmGetResource(db, "Xft.dpi", "String", &type, &value) && value.addr) {
        double dpi = std::atof(value.addr);
        if (dpi > 0) scale = static_cast<float>(dpi / 96.0);
    }
    XrmDestroyDatabase(db);
    return scale;
}

nostd::Vector<VeraMonitorInfo> queryMonitorsX11(X11Context& ctx) {
    nostd::Vector<VeraMonitorInfo> out;

    int monitorCount = 0;
    XRRMonitorInfo* monitors =
        XRRGetMonitors(ctx.display, ctx.root, True, &monitorCount);
    if (!monitors) return out;

    Atom actualType;
    int actualFormat;
    ulong itemCount, bytesAfter;
    unsigned char* workAreaProp = nullptr;
    int64_t workX = 0, workY = 0, workW = 0, workH = 0;
    if (XGetWindowProperty(ctx.display, ctx.root, ctx.atoms.netWorkarea, 0, 4,
                           False, AnyPropertyType, &actualType, &actualFormat,
                           &itemCount, &bytesAfter, &workAreaProp) == Success &&
        workAreaProp && itemCount >= 4) {
        int64_t* values = reinterpret_cast<int64_t*>(workAreaProp);
        workX = values[0];
        workY = values[1];
        workW = values[2];
        workH = values[3];
    }
    if (workAreaProp) XFree(workAreaProp);

    float dpiScale = queryDpiScaleX11(ctx);

    static const char* sfallbackNames[] = {
        "monitor-0", "monitor-1", "monitor-2", "monitor-3",
        "monitor-4", "monitor-5", "monitor-6", "monitor-7"};

    for (int i = 0; i < monitorCount; ++i) {
        const XRRMonitorInfo& m = monitors[i];
        char* atomName = XGetAtomName(ctx.display, m.name);

        VeraMonitorInfo info;
        if (atomName) {
            info.name = atomName;

        } else {
            info.name = (i < 8) ? sfallbackNames[i] : "monitor-generic";
        }
        info.x = m.x;
        info.y = m.y;
        info.workAreaX = workW > 0 ? static_cast<int32_t>(workX) : m.x;
        info.workAreaY = workH > 0 ? static_cast<int32_t>(workY) : m.y;
        info.workAreaWidth = workW > 0 ? static_cast<uint32_t>(workW)
                                       : static_cast<uint32_t>(m.width);
        info.workAreaHeight = workH > 0 ? static_cast<uint32_t>(workH)
                                        : static_cast<uint32_t>(m.height);
        info.dpiScale = dpiScale;
        info.isPrimary = m.primary != 0;
        if (m.mwidth > 0 && m.mheight > 0) {
            info.physicalWidthMm = static_cast<uint32_t>(m.mwidth);
            info.physicalHeightMm = static_cast<uint32_t>(m.mheight);
        }

        info.refreshRateHz = 60;
        XRRScreenResources* res =
            XRRGetScreenResourcesCurrent(ctx.display, ctx.root);
        if (res && m.noutput > 0) {
            XRROutputInfo* outputInfo =
                XRRGetOutputInfo(ctx.display, res, m.outputs[0]);
            if (outputInfo && outputInfo->crtc) {
                XRRCrtcInfo* crtcInfo =
                    XRRGetCrtcInfo(ctx.display, res, outputInfo->crtc);
                if (crtcInfo) {
                    for (int mi = 0; mi < res->nmode; ++mi) {
                        if (res->modes[mi].id == crtcInfo->mode) {
                            const XRRModeInfo& mode = res->modes[mi];
                            if (mode.hTotal && mode.vTotal) {
                                info.refreshRateHz =
                                    static_cast<uint32_t>(std::round(
                                        static_cast<double>(mode.dotClock) /
                                        (static_cast<double>(mode.hTotal) *
                                         mode.vTotal)));
                            }
                            break;
                        }
                    }
                    XRRFreeCrtcInfo(crtcInfo);
                }
            }
            if (outputInfo) XRRFreeOutputInfo(outputInfo);
            XRRFreeScreenResources(res);
        }

        if (atomName) XFree(atomName);
        out.push_back(std::move(info));
    }

    XRRFreeMonitors(monitors);
    return out;
}

nostd::Vector<VeraDisplayModeInfo> queryDisplayModesX11(
    X11Context& ctx, const VeraMonitorInfo& monitor) {
    nostd::Vector<VeraDisplayModeInfo> out;

    int monitorCount = 0;
    XRRMonitorInfo* monitors =
        XRRGetMonitors(ctx.display, ctx.root, True, &monitorCount);
    if (!monitors) return out;

    for (int i = 0; i < monitorCount; ++i) {
        char* name = XGetAtomName(ctx.display, monitors[i].name);
        bool match = name && monitor.name == name;
        if (name) XFree(name);
        if (!match || monitors[i].noutput == 0) continue;

        XRRScreenResources* res =
            XRRGetScreenResourcesCurrent(ctx.display, ctx.root);
        if (!res) break;
        XRROutputInfo* outputInfo =
            XRRGetOutputInfo(ctx.display, res, monitors[i].outputs[0]);
        if (outputInfo) {
            for (int m = 0; m < outputInfo->nmode; ++m) {
                for (int mi = 0; mi < res->nmode; ++mi) {
                    if (res->modes[mi].id != outputInfo->modes[m]) continue;
                    const XRRModeInfo& mode = res->modes[mi];
                    uint32_t refresh = 60;
                    if (mode.hTotal && mode.vTotal) {
                        refresh = static_cast<uint32_t>(std::round(
                            static_cast<double>(mode.dotClock) /
                            (static_cast<double>(mode.hTotal) * mode.vTotal)));
                    }
                    out.push_back(VeraDisplayModeInfo{
                        .width = mode.width,
                        .height = mode.height,
                        .refreshRateHz = refresh,
                        .bitsPerPixel = 32,
                    });
                }
            }
            XRRFreeOutputInfo(outputInfo);
        }
        XRRFreeScreenResources(res);
        break;
    }

    XRRFreeMonitors(monitors);
    return out;
}

#include <X11/Xlib.h>

#include <cstdint>

struct MotifWmHints {
    ulong flags;
    ulong functions;
    ulong decorations;
    int64_t inputMode;
    ulong status;
};

static constexpr ulong MWM_HINTS_DECORATIONS = 1L << 1;

void setDecoratedX11(X11Context& ctx, Window window, bool decorated) {
    MotifWmHints hints{};
    hints.flags = MWM_HINTS_DECORATIONS;
    hints.decorations = decorated ? 1 : 0;

    XChangeProperty(ctx.display, window, ctx.atoms.motifWmHints,
                    ctx.atoms.motifWmHints, 32, PropModeReplace,
                    reinterpret_cast<unsigned char*>(&hints), 5);
}

static bool pointInRect(int32_t x, int32_t y, const VeraRect& r) {
    return x >= static_cast<int32_t>(r.x) &&
           x < static_cast<int32_t>(r.x + r.width) &&
           y >= static_cast<int32_t>(r.y) &&
           y < static_cast<int32_t>(r.y + r.height);
}

void handleTitlebarButtonPressX11(X11Context& ctx, Window window,
                                  const VeraHitTestRegions& regions,
                                  int32_t clickX, int32_t clickY) {
    if (!regions.dragRegion ||
        !pointInRect(clickX, clickY, *regions.dragRegion)) {
        return;
    }

    if (regions.minimizeButton &&
        pointInRect(clickX, clickY, *regions.minimizeButton)) {
        return;
    }
    if (regions.maximizeButton &&
        pointInRect(clickX, clickY, *regions.maximizeButton)) {
        return;
    }
    if (regions.closeButton &&
        pointInRect(clickX, clickY, *regions.closeButton)) {
        return;
    }

    constexpr int64_t moveResizeMove = 8;

    Window child;
    int rootX = 0, rootY = 0, winX = 0, winY = 0;
    unsigned int mask;
    ::Window rootReturn;
    XQueryPointer(ctx.display, window, &rootReturn, &child, &rootX, &rootY,
                  &winX, &winY, &mask);

    XEvent event{};
    event.type = ClientMessage;
    event.xclient.window = window;
    event.xclient.message_type = ctx.atoms.netWmMoveresize;
    event.xclient.format = 32;
    event.xclient.data.l[0] = rootX;
    event.xclient.data.l[1] = rootY;
    event.xclient.data.l[2] = moveResizeMove;
    event.xclient.data.l[3] = Button1;
    event.xclient.data.l[4] = 1;

    XUngrabPointer(ctx.display, CurrentTime);
    XSendEvent(ctx.display, ctx.root, False,
               SubstructureRedirectMask | SubstructureNotifyMask, &event);
}

static void requestNativeRefreshMode(X11Context& ctx) {
    XRRScreenResources* res =
        XRRGetScreenResourcesCurrent(ctx.display, ctx.root);
    if (!res) return;

    for (int i = 0; i < res->noutput; ++i) {
        XRROutputInfo* output =
            XRRGetOutputInfo(ctx.display, res, res->outputs[i]);
        if (!output || output->connection != RR_Connected || !output->crtc) {
            if (output) XRRFreeOutputInfo(output);
            continue;
        }
        XRRCrtcInfo* crtc = XRRGetCrtcInfo(ctx.display, res, output->crtc);
        if (crtc) {
            RRMode bestMode = crtc->mode;
            double bestRefresh = 0;
            for (int m = 0; m < output->nmode; ++m) {
                for (int mi = 0; mi < res->nmode; ++mi) {
                    if (res->modes[mi].id != output->modes[m]) continue;
                    const XRRModeInfo& mode = res->modes[mi];
                    if (mode.width != crtc->width ||
                        mode.height != crtc->height) {
                        continue;
                    }
                    double refresh =
                        mode.hTotal && mode.vTotal
                            ? mode.dotClock /
                                  (static_cast<double>(mode.hTotal) *
                                   mode.vTotal)
                            : 0;
                    if (refresh > bestRefresh) {
                        bestRefresh = refresh;
                        bestMode = mode.id;
                    }
                }
            }
            if (bestMode != crtc->mode) {
                XRRSetCrtcConfig(ctx.display, res, output->crtc, CurrentTime,
                                 crtc->x, crtc->y, bestMode, crtc->rotation,
                                 crtc->outputs, crtc->noutput);
            }
            XRRFreeCrtcInfo(crtc);
        }
        XRRFreeOutputInfo(output);
    }
    XRRFreeScreenResources(res);
}

void applyFullscreenX11(X11Context& ctx, Window window, FullScreenMode mode) {
    switch (mode) {
        case FullScreenMode::Windowed:
            setNetWmStateX11(ctx, window, ctx.atoms.netWmStateFullscreen, 0,
                             /*add=*/false);
            break;
        case FullScreenMode::Borderless:
            setNetWmStateX11(ctx, window, ctx.atoms.netWmStateFullscreen, 0,
                             /*add=*/true);
            break;
        case FullScreenMode::Exclusive:
            setNetWmStateX11(ctx, window, ctx.atoms.netWmStateFullscreen, 0,
                             /*add=*/true);
            requestNativeRefreshMode(ctx);
            break;
    }
}

void setTitleX11(X11Context& ctx, Window window, const char* title) {
    if (!title) return;

    XStoreName(ctx.display, window, title);  // legacy WM_NAME fallback
    XChangeProperty(ctx.display, window, ctx.atoms.netWmName,
                    ctx.atoms.utf8String, 8, PropModeReplace,
                    reinterpret_cast<const unsigned char*>(title),
                    static_cast<int>(strlen(title)));
}

void setIconX11(X11Context& ctx, Window window, const char* iconPath) {
    if (!iconPath || *iconPath == '\0') return;
    (void)ctx;
    (void)window;
}

void setSizeHintsX11(X11Context& ctx, Window window, uint32_t minWidth,
                     uint32_t minHeight, uint32_t maxWidth, uint32_t maxHeight,
                     bool resizable) {
    XSizeHints* hints = XAllocSizeHints();
    hints->flags = PMinSize | PMaxSize;
    hints->min_width = static_cast<int>(minWidth);
    hints->min_height = static_cast<int>(minHeight);

    if (!resizable) {
        hints->max_width = static_cast<int>(maxWidth > 0 ? maxWidth : minWidth);
        hints->max_height =
            static_cast<int>(maxHeight > 0 ? maxHeight : minHeight);
    } else {
        hints->max_width =
            maxWidth > 0 ? static_cast<int>(maxWidth) : INT16_MAX;
        hints->max_height =
            maxHeight > 0 ? static_cast<int>(maxHeight) : INT16_MAX;
    }

    XSetWMNormalHints(ctx.display, window, hints);
    XFree(hints);
}

void setNetWmStateX11(X11Context& ctx, Window window, Atom state1, Atom state2,
                      bool add) {
    constexpr int64_t netWmStateRemove = 0, netWmStateAdd = 1;

    XEvent event{};
    event.type = ClientMessage;
    event.xclient.window = window;
    event.xclient.message_type = ctx.atoms.netWmState;
    event.xclient.format = 32;
    event.xclient.data.l[0] = add ? netWmStateAdd : netWmStateRemove;
    event.xclient.data.l[1] = static_cast<int64_t>(state1);
    event.xclient.data.l[2] = static_cast<int64_t>(state2);
    event.xclient.data.l[3] = 1;  // source indication: normal application

    XSendEvent(ctx.display, ctx.root, False,
               SubstructureRedirectMask | SubstructureNotifyMask, &event);
}

void setAlwaysOnTopX11(X11Context& ctx, Window window, bool value) {
    setNetWmStateX11(ctx, window, ctx.atoms.netWmStateAbove, 0, value);
}

void setWindowTypeX11(X11Context& ctx, Window window) {
    XChangeProperty(
        ctx.display, window, ctx.atoms.netWmWindowType, XA_ATOM, 32,
        PropModeReplace,
        reinterpret_cast<unsigned char*>(&ctx.atoms.netWmWindowTypeNormal), 1);
}

void setPidX11(X11Context& ctx, Window window) {
    int64_t pid = static_cast<int64_t>(getpid());
    XChangeProperty(ctx.display, window, ctx.atoms.netWmPid, XA_CARDINAL, 32,
                    PropModeReplace, reinterpret_cast<unsigned char*>(&pid), 1);
}

bool hasNetWmStateX11(X11Context& ctx, Window window, Atom state) {
    Atom actualType;
    int actualFormat;
    ulong itemCount, bytesAfter;
    unsigned char* data = nullptr;
    bool found = false;

    if (XGetWindowProperty(ctx.display, window, ctx.atoms.netWmState, 0, 1024,
                           False, XA_ATOM, &actualType, &actualFormat,
                           &itemCount, &bytesAfter, &data) == Success &&
        data) {
        Atom* atoms = reinterpret_cast<Atom*>(data);
        for (ulong i = 0; i < itemCount; ++i) {
            if (atoms[i] == state) {
                found = true;
                break;
            }
        }
        XFree(data);
    }
    return found;
}

#include <X11/Xatom.h>

#include <cstdint>

#undef Status
#undef Bool

X11Window::X11Window(X11Context& ctx, Window xid, VeraWindowHandle handle,
                     const VeraWindowInfo& info)
    : m_ctx(ctx),
      m_xid(xid),
      m_handle(handle),
      m_customTitleBar(info.customTitleBar) {
    m_state.width = info.width;
    m_state.height = info.height;
    m_state.x = info.x.value_or(0);
    m_state.y = info.y.value_or(0);
    m_state.isVisible = info.startVisible;

    ctx.windowsByXid.push_back({xid, this});

    XSelectInput(ctx.display, xid,
                 ExposureMask | StructureNotifyMask | FocusChangeMask |
                     KeyPressMask | KeyReleaseMask | ButtonPressMask |
                     ButtonReleaseMask | PointerMotionMask |
                     PropertyChangeMask);

    XSetWMProtocols(ctx.display, xid, &ctx.atoms.wmDeleteWindow, 1);

    if (ctx.xim) {
        m_xic = XCreateIC(ctx.xim, XNInputStyle,
                          XIMPreeditNothing | XIMStatusNothing, XNClientWindow,
                          m_xid, XNFocusWindow, m_xid, nullptr);
    }

    setTitleX11(ctx, xid, info.title);
    setSizeHintsX11(ctx, xid, info.minWidth, info.minHeight, info.maxWidth,
                    info.maxHeight, info.resizable);
    setWindowTypeX11(ctx, xid);
    setPidX11(ctx, xid);
    if (!info.decorated) setDecoratedX11(ctx, xid, false);
    if (info.alwaysOnTop) setAlwaysOnTopX11(ctx, xid, true);
    if (info.iconPath) setIconX11(ctx, xid, info.iconPath);

    Atom xdndVersion = 5;
    XChangeProperty(ctx.display, xid, ctx.atoms.xdndAware, XA_ATOM, 32,
                    PropModeReplace,
                    reinterpret_cast<unsigned char*>(&xdndVersion), 1);

    if (info.startVisible) show();
    if (info.startMaximized) maximize();
    if (info.startMinimized) minimize();
    if (info.fullscreenMode != FullScreenMode::Windowed) {
        setFullscreen(info.fullscreenMode);
    }
}

X11Window::~X11Window() {
    if (m_xic) {
        XDestroyIC(m_xic);
        m_xic = nullptr;
    }

    for (size_t i = 0; i < m_ctx.windowsByXid.size(); ++i) {
        if (m_ctx.windowsByXid[i].window == this) {
            m_ctx.windowsByXid[i] = m_ctx.windowsByXid.back();
            m_ctx.windowsByXid.resize(m_ctx.windowsByXid.size() - 1);
            break;
        }
    }

    XDestroyWindow(m_ctx.display, m_xid);
}

VeraWindowHandle X11Window::getHandle() const { return m_handle; }

VeraNativeHandle X11Window::getNativeHandle() const {
    VeraNativeHandle handle;
    handle.display = m_ctx.display;
    handle.x11Window = static_cast<uint64_t>(m_xid);
    return handle;
}

void X11Window::setSize(uint32_t width, uint32_t height) {
    XResizeWindow(m_ctx.display, m_xid, width, height);
}
void X11Window::setPosition(int32_t x, int32_t y) {
    XMoveWindow(m_ctx.display, m_xid, x, y);
}
void X11Window::setMinSize(uint32_t width, uint32_t height) {
    setSizeHintsX11(m_ctx, m_xid, width, height, 0, 0, true);
}
void X11Window::setMaxSize(uint32_t width, uint32_t height) {
    setSizeHintsX11(m_ctx, m_xid, 0, 0, width, height, true);
}
VeraWindowState X11Window::getState() const { return m_state; }

void X11Window::show() { XMapWindow(m_ctx.display, m_xid); }
void X11Window::hide() { XUnmapWindow(m_ctx.display, m_xid); }
void X11Window::minimize() {
    XIconifyWindow(m_ctx.display, m_xid, m_ctx.screen);
}
void X11Window::maximize() {
    setNetWmStateX11(m_ctx, m_xid, m_ctx.atoms.netWmStateMaximizedHorz,
                     m_ctx.atoms.netWmStateMaximizedVert, true);
}
void X11Window::restore() {
    setNetWmStateX11(m_ctx, m_xid, m_ctx.atoms.netWmStateMaximizedHorz,
                     m_ctx.atoms.netWmStateMaximizedVert, false);
    XMapWindow(m_ctx.display, m_xid);
}

void X11Window::close() {
    if (!m_closeRequestCallback || m_closeRequestCallback()) {
        XEvent event{};
        event.type = ClientMessage;
        event.xclient.window = m_xid;
        event.xclient.message_type = m_ctx.atoms.wmProtocols;
        event.xclient.format = 32;
        event.xclient.data.l[0] =
            static_cast<int64_t>(m_ctx.atoms.wmDeleteWindow);
        XSendEvent(m_ctx.display, m_xid, False, NoEventMask, &event);
    }
}

void X11Window::handleWmCloseRequest() {
    if (m_closeRequestCallback) {
        if (!m_closeRequestCallback()) return;
    }

    m_pendingDeletion = true;
}

void X11Window::focus() {
    XSetInputFocus(m_ctx.display, m_xid, RevertToParent, CurrentTime);
    XRaiseWindow(m_ctx.display, m_xid);
}
void X11Window::setTitle(const char* title) {
    setTitleX11(m_ctx, m_xid, title);
}
void X11Window::setFullscreen(FullScreenMode mode) {
    applyFullscreenX11(m_ctx, m_xid, mode);
    m_state.isFullscreen = (mode != FullScreenMode::Windowed);
}
void X11Window::setAlwaysOnTop(bool value) {
    setAlwaysOnTopX11(m_ctx, m_xid, value);
}
void X11Window::setIcon(const char* iconPath) {
    setIconX11(m_ctx, m_xid, iconPath);
}

void X11Window::setTitlebarHitTestRegions(const VeraHitTestRegions& regions) {
    m_hitTestRegions = regions;
}

void X11Window::setResizeCallback(
    nostd::Function<void(uint32_t, uint32_t)> callback) {
    m_resizeCallback = std::move(callback);
}
void X11Window::setMoveCallback(
    nostd::Function<void(int32_t, int32_t)> callback) {
    m_moveCallback = std::move(callback);
}
void X11Window::setCloseRequestCallback(nostd::Function<bool()> callback) {
    m_closeRequestCallback = std::move(callback);
}
void X11Window::setFocusChangeCallback(nostd::Function<void(bool)> callback) {
    m_focusChangeCallback = std::move(callback);
}
void X11Window::setDpiChangeCallback(nostd::Function<void(float)> callback) {
    m_dpiChangeCallback = std::move(callback);
}

void X11Window::setKeyCallback(
    nostd::Function<void(VeraKey, bool, bool)> callback) {
    m_keyCallback = std::move(callback);
}
void X11Window::setMouseButtonCallback(
    nostd::Function<void(VeraMouseButton, bool)> callback) {
    m_mouseButtonCallback = std::move(callback);
}
void X11Window::setMouseMoveCallback(
    nostd::Function<void(double, double)> callback) {
    m_mouseMoveCallback = std::move(callback);
}
void X11Window::setScrollCallback(
    nostd::Function<void(double, double)> callback) {
    m_scrollCallback = std::move(callback);
}
void X11Window::setCharCallback(nostd::Function<void(const char*)> callback) {
    m_charCallback = std::move(callback);
}

void X11Window::setCursorMode(VeraCursorMode mode) {
    m_cursorDisabled = (mode == VeraCursorMode::Disabled);
    applyCursorModeX11(m_ctx, m_xid, mode);
}
void X11Window::setCursorShape(VeraCursorShape shape) {
    applyCursorShapeX11(m_ctx, m_xid, shape);
}

VeraMonitorInfo X11Window::getCurrentMonitor() const {
    int32_t centerX = m_state.x + static_cast<int32_t>(m_state.width) / 2;
    int32_t centerY = m_state.y + static_cast<int32_t>(m_state.height) / 2;
    return getMonitorAtCoordinateXYX11(m_ctx, centerX, centerY);
}

void X11Window::setJoystickButtonCallback(VeraJoystickButtonCallback callback) {
    m_joyButtonCallback = callback;
    setJoystickButtonCallbackX11(
        [this](uint32_t id, uint32_t btn, bool pressed) {
            if (m_joyButtonCallback) m_joyButtonCallback(id, btn, pressed);
        });
}

void X11Window::setJoystickAxisCallback(VeraJoystickAxisCallback callback) {
    m_joyAxisCallback = callback;
    setJoystickAxisCallbackX11([this](uint32_t id, uint32_t axis, float val) {
        if (m_joyAxisCallback) m_joyAxisCallback(id, axis, val);
    });
}

void X11Window::handleXEvent(XEvent& event) {
    switch (event.type) {
        case ConfigureNotify: {
            auto& xc = event.xconfigure;
            if (static_cast<uint32_t>(xc.width) != m_state.width ||
                static_cast<uint32_t>(xc.height) != m_state.height) {
                m_state.width = xc.width;
                m_state.height = xc.height;
                if (m_resizeCallback) {
                    m_resizeCallback(m_state.width, m_state.height);
                }
            }
            if (xc.x != m_state.x || xc.y != m_state.y) {
                m_state.x = xc.x;
                m_state.y = xc.y;
                if (m_moveCallback) m_moveCallback(m_state.x, m_state.y);
            }
            break;
        }
        case MapNotify:
            m_state.isVisible = true;
            m_state.isMinimized = false;
            break;
        case UnmapNotify:
            m_state.isVisible = false;
            break;
        case FocusIn:
            m_state.isFocused = true;
            if (m_xic) XSetICFocus(m_xic);  // Set layout focus
            if (m_focusChangeCallback) m_focusChangeCallback(true);
            break;
        case FocusOut:
            m_state.isFocused = false;
            if (m_xic) XUnsetICFocus(m_xic);  // Unset layout focus
            if (m_focusChangeCallback) m_focusChangeCallback(false);
            break;
        case KeyPress:
            handleKeyPressX11(m_ctx, event.xkey, m_keyState, m_keyCallback,
                              m_charCallback);
            break;
        case KeyRelease:
            handleKeyReleaseX11(m_ctx, event.xkey, m_keyState, m_keyCallback);
            break;
        case ButtonPress:
            if (m_customTitleBar) {
                handleTitlebarButtonPressX11(m_ctx, m_xid, m_hitTestRegions,
                                             event.xbutton.x, event.xbutton.y);
            }
            handleMouseButtonPressX11(event.xbutton, m_mouseButtonCallback,
                                      m_scrollCallback);
            break;
        case ButtonRelease:
            handleMouseButtonReleaseX11(event.xbutton, m_mouseButtonCallback);
            break;
        case MotionNotify:
            if (m_cursorDisabled) {
                int centerX = static_cast<int>(m_state.width) / 2;
                int centerY = static_cast<int>(m_state.height) / 2;
                double dx = event.xmotion.x - centerX;
                double dy = event.xmotion.y - centerY;
                if ((dx != 0 || dy != 0) && m_mouseMoveCallback) {
                    m_mouseMoveCallback(dx, dy);
                }
                XWarpPointer(m_ctx.display, None, m_xid, 0, 0, 0, 0, centerX,
                             centerY);
            } else if (m_mouseMoveCallback) {
                m_mouseMoveCallback(event.xmotion.x, event.xmotion.y);
            }
            break;
        case PropertyNotify:
            if (event.xproperty.atom == m_ctx.atoms.netWmState) {
                m_state.isMaximized = hasNetWmStateX11(
                    m_ctx, m_xid, m_ctx.atoms.netWmStateMaximizedHorz);
                m_state.isFullscreen = hasNetWmStateX11(
                    m_ctx, m_xid, m_ctx.atoms.netWmStateFullscreen);
            }
            break;
        default:
            break;
    }
}

void X11Window::setDestructionCallback(
    nostd::Function<void(VeraWindow*)> callback) {
    (void)callback;
}

// Map platform-specific enum entries to sequential linux joydev button numbers
static constexpr uint8_t mapToLinuxJoydevButton(VeraJoystickButton button) {
    switch (button) {
        // Face Buttons
        case VeraJoystickButton::Cross:
        case VeraJoystickButton::XboxA:
            return 0;
        case VeraJoystickButton::Circle:
        case VeraJoystickButton::XboxB:
            return 1;
        case VeraJoystickButton::Square:
        case VeraJoystickButton::XboxX:
            return 2;
        case VeraJoystickButton::Triangle:
        case VeraJoystickButton::XboxY:
            return 3;

        // Bumpers
        case VeraJoystickButton::L1:
        case VeraJoystickButton::XboxLB:
            return 4;
        case VeraJoystickButton::R1:
        case VeraJoystickButton::XboxRB:
            return 5;

        // Triggers (Digital/Click Emulation)
        case VeraJoystickButton::L2:
        case VeraJoystickButton::XboxLT:
            return 6;
        case VeraJoystickButton::R2:
        case VeraJoystickButton::XboxRT:
            return 7;

        // Menu / System Options
        case VeraJoystickButton::Share:
        case VeraJoystickButton::XboxBack:
            return 8;
        case VeraJoystickButton::Options:
        case VeraJoystickButton::XboxStart:
            return 9;
        case VeraJoystickButton::PS:
        case VeraJoystickButton::XboxGuide:
            return 10;

        // Stick Clicks
        case VeraJoystickButton::L3:
        case VeraJoystickButton::XboxLS:
            return 11;
        case VeraJoystickButton::R3:
        case VeraJoystickButton::XboxRS:
            return 12;

        // Directional D-Pad Fallbacks
        case VeraJoystickButton::DpadUp:
            return 13;
        case VeraJoystickButton::DpadDown:
            return 14;
        case VeraJoystickButton::DpadLeft:
            return 15;
        case VeraJoystickButton::DpadRight:
            return 16;

        // Platform Exclusives
        case VeraJoystickButton::Touchpad:
            return 17;
        case VeraJoystickButton::XboxShare:
            return 18;

        default:
            return 255;
    }
}

bool X11Window::isPressed(const VeraPressable& input) const {
    // Rule: If this specific window does not have active system focus,
    // it shouldn't respond to any input states.
    if (!this->m_state.isFocused) {
        return false;
    }

    return std::visit(
        [this](auto&& arg) -> bool {
            using T = std::decay_t<decltype(arg)>;

            // 1. Keyboard State (Read from central context, filtered by window
            // focus)
            if constexpr (std::is_same_v<T, VeraKey>) {
                if (arg == VeraKey::Unknown || arg == VeraKey::Count) {
                    return false;
                }
                size_t keyIndex = static_cast<size_t>(arg);
                if (keyIndex < std::size(this->m_ctx.keyStates)) {
                    return this->m_ctx.keyStates[keyIndex];
                }
                return false;
            }

            // 2. Mouse Button State (Read from central context, filtered by
            // window focus)
            else if constexpr (std::is_same_v<T, VeraMouseButton>) {
                if (arg == VeraMouseButton::Count) return false;
                size_t mouseIndex = static_cast<size_t>(arg);
                if (mouseIndex < std::size(this->m_ctx.mouseButtonStates)) {
                    return this->m_ctx.mouseButtonStates[mouseIndex];
                }
                return false;
            }

            // 3. Gamepad Evaluation (Read via cross-file lookup, filtered by
            // window focus)
            else if constexpr (std::is_same_v<T, VeraJoystickButton>) {
                if (arg == VeraJoystickButton::Count) return false;
                uint8_t rawButtonId = mapToLinuxJoydevButton(arg);
                if (rawButtonId == 255) return false;

                // Safely cross-query slot 0 via your existing global X11
                // joystick header system
                VeraJoystickState joyState =
                    getJoystickStateX11(this->m_ctx, 0);
                if (joyState.connected &&
                    rawButtonId < joyState.buttons.size()) {
                    return joyState.buttons[rawButtonId];
                }
                return false;
            }

            return false;
        },
        input);
}

bool initializeXInputX11(X11Context& ctx, int& outOpcode) {
    int event = 0;
    int error = 0;

    if (!XQueryExtension(ctx.display, "XInputExtension", &outOpcode, &event,
                         &error)) {
        return false;
    }

    int major = 2;
    int minor = 0;
    if (XIQueryVersion(ctx.display, &major, &minor) == BadRequest) {
        return false;
    }

    XIEventMask mask{};
    unsigned char maskBits[XIMaskLen(XI_HierarchyChanged)] = {0};

    mask.deviceid = XIAllDevices;
    mask.mask_len = static_cast<int>(sizeof(maskBits));
    mask.mask = maskBits;

    XISetMask(maskBits, XI_HierarchyChanged);
    XISelectEvents(ctx.display, ctx.root, &mask, 1);

    return true;
}

nostd::Vector<VeraInputDeviceInfo> enumerateInputDevicesX11(X11Context& ctx) {
    nostd::Vector<VeraInputDeviceInfo> devices;
    int count = 0;

    XIDeviceInfo* info = XIQueryDevice(ctx.display, XIAllDevices, &count);
    if (!info) {
        return devices;
    }

    for (int i = 0; i < count; ++i) {
        if (info[i].use != XIMasterPointer && info[i].use != XIMasterKeyboard) {
            continue;
        }

        const char* rawName = info[i].name ? info[i].name : "Unknown device";

        VeraInputDeviceInfo dev{};
        dev.name =
            rawName;  // Direct assignment using string constructor/operator=
        dev.connected = (info[i].enabled != 0);

        devices.push_back(dev);
    }

    XIFreeDeviceInfo(info);
    return devices;
}

X11Window* findWindowByXid(const X11Context& ctx, Window xid) {
    for (size_t i = 0; i < ctx.windowsByXid.size(); ++i) {
        if (ctx.windowsByXid[i].xid == xid) {
            return ctx.windowsByXid[i].window;
        }
    }
    return nullptr;
}
