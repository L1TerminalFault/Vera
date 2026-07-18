#include "X11DragDrop.hxx"

#include <X11/Xlib.h>

#include <cstdint>
#include <sstream>

#include "core/window/Window.h"

static VeraDragCallback gCallback;
static Window gSourceWindow = 0;
static int32_t gPendingX = 0, gPendingY = 0;

void setCallback(VeraDragCallback callback) { gCallback = std::move(callback); }

static std::vector<std::string> parseUriList(const std::string& data) {
    std::vector<std::string> paths;
    std::istringstream stream(data);
    std::string line;
    while (std::getline(stream, line)) {
        if (line.empty() || line[0] == '#') continue;

        constexpr char kFilePrefix[] = "file://";
        if (line.rfind(kFilePrefix, 0) == 0) {
            line = line.substr(sizeof(kFilePrefix) - 1);
        }
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (!line.empty()) paths.push_back(line);
    }
    return paths;
}

void handleClientMessage(X11Context& ctx, VeraWindow* window,
                         XClientMessageEvent& event) {
    if (!gCallback) return;

    if (event.message_type == ctx.atoms.xdndEnter) {
        gSourceWindow = static_cast<Window>(event.data.l[0]);
        gCallback(VeraDragEvent{VeraDragAction::Enter, window, 0, 0, {}});
    } else if (event.message_type == ctx.atoms.xdndPosition) {
        int32_t rootX = static_cast<int32_t>(event.data.l[2]) >> 16;
        int32_t rootY = static_cast<int32_t>(event.data.l[2]) & 0xFFFF;
        gPendingX = rootX;
        gPendingY = rootY;
        bool accept = gCallback(
            VeraDragEvent{VeraDragAction::Over, window, rootX, rootY, {}});

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
        gCallback(VeraDragEvent{VeraDragAction::Leave, window, 0, 0, {}});
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
        std::string uriList(reinterpret_cast<char*>(data), itemCount);
        XFree(data);

        VeraDragEvent dropEvent{VeraDragAction::Drop, window, gPendingX,
                                gPendingY, parseUriList(uriList)};
        gCallback(dropEvent);
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
