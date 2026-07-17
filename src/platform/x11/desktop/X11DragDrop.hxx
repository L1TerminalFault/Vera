#pragma once

#include <X11/Xlib.h>

// INFO: Erasing the polluting X11 preprocessor macros completely
// #undef Status
// #undef Success
// #undef Bool
// #undef True
// #undef False
// #undef None
// #undef CursorShape
// #undef Button1
// #undef Button2
// #undef Button3
// #undef Button4
// #undef Button5

#include <sstream>

#include "core/window/DragDrop.h"
#include "core/window/Window.h"
#include "platform/x11/internal/X11Internal.hxx"

namespace vera::x11::desktop::dnd {

using namespace core::window;
using namespace internal;

static VeraDragCallback g_callback;
static Window g_sourceWindow = 0;
static int32_t g_pendingX = 0, g_pendingY = 0;

void setCallback(VeraDragCallback callback) {
    g_callback = std::move(callback);
}

static std::vector<std::string> parseUriList(const std::string& data) {
    std::vector<std::string> paths;
    std::istringstream stream(data);
    std::string line;
    while (std::getline(stream, line)) {
        if (line.empty() || line[0] == '#') continue;
        // Strip the file:// scheme; leave everything else (including
        // percent-encoding) to the caller since decoding it is a text-utility
        // concern, not a windowing one.
        constexpr char kFilePrefix[] = "file://";
        if (line.rfind(kFilePrefix, 0) == 0)
            line = line.substr(sizeof(kFilePrefix) - 1);
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (!line.empty()) paths.push_back(line);
    }
    return paths;
}

void handleClientMessage(X11Context& ctx, VeraWindow* window,
                         XClientMessageEvent& event) {
    if (!g_callback) return;

    if (event.message_type == ctx.atoms.xdndEnter) {
        g_sourceWindow = static_cast<Window>(event.data.l[0]);
        g_callback(VeraDragEvent{VeraDragAction::Enter, window, 0, 0, {}});
    } else if (event.message_type == ctx.atoms.xdndPosition) {
        int32_t rootX = static_cast<int32_t>(event.data.l[2]) >> 16;
        int32_t rootY = static_cast<int32_t>(event.data.l[2]) & 0xFFFF;
        g_pendingX = rootX;
        g_pendingY = rootY;
        bool accept = g_callback(
            VeraDragEvent{VeraDragAction::Over, window, rootX, rootY, {}});

        XClientMessageEvent status{};
        status.type = ClientMessage;
        status.window = event.data.l[0];
        status.message_type = ctx.atoms.xdndStatus;
        status.format = 32;
        status.data.l[0] =
            static_cast<long>(window->getNativeHandle().x11Window);
        status.data.l[1] = accept ? 1 : 0;
        status.data.l[4] =
            accept ? static_cast<long>(ctx.atoms.xdndActionCopy) : 0;
        XSendEvent(ctx.display, event.data.l[0], False, NoEventMask,
                   reinterpret_cast<XEvent*>(&status));
    } else if (event.message_type == ctx.atoms.xdndDrop) {
        // Request the file list; the payload arrives as a SelectionNotify.
        Window target =
            static_cast<Window>(window->getNativeHandle().x11Window);
        XConvertSelection(ctx.display, ctx.atoms.xdndSelection,
                          ctx.atoms.textUriList, ctx.atoms.xdndSelection,
                          target, event.data.l[2] /* timestamp */);
    } else if (event.message_type == ctx.atoms.xdndLeave) {
        g_callback(VeraDragEvent{VeraDragAction::Leave, window, 0, 0, {}});
    }
}

void handleSelectionNotify(X11Context& ctx, VeraWindow* window,
                           XSelectionEvent& event) {
    if (event.property == None || !g_callback) return;

    Atom actualType;
    int actualFormat;
    unsigned long itemCount, bytesAfter;
    unsigned char* data = nullptr;
    Window target = static_cast<Window>(window->getNativeHandle().x11Window);

    if (XGetWindowProperty(ctx.display, target, event.property, 0, 1 << 20,
                           True, AnyPropertyType, &actualType, &actualFormat,
                           &itemCount, &bytesAfter, &data) == Success &&
        data) {
        std::string uriList(reinterpret_cast<char*>(data), itemCount);
        XFree(data);

        VeraDragEvent dropEvent{VeraDragAction::Drop, window, g_pendingX,
                                g_pendingY, parseUriList(uriList)};
        g_callback(dropEvent);
    }

    XClientMessageEvent finished{};
    finished.type = ClientMessage;
    finished.window = g_sourceWindow;
    finished.message_type = ctx.atoms.xdndFinished;
    finished.format = 32;
    finished.data.l[0] = static_cast<long>(target);
    finished.data.l[1] = 1;
    finished.data.l[2] = static_cast<long>(ctx.atoms.xdndActionCopy);
    XSendEvent(ctx.display, g_sourceWindow, False, NoEventMask,
               reinterpret_cast<XEvent*>(&finished));
}

}  // namespace vera::x11::desktop::dnd
