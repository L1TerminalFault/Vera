#pragma once
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <unistd.h>

#include <string>

#include "platform/x11/internal/X11Internal.hxx"

static std::string gOwnedText;
static bool gOwningSelection = false;

namespace clipboard {

void initialize(X11Context& ctx) {
    ctx.clipboardOwnerWindow =
        XCreateSimpleWindow(ctx.display, ctx.root, -10, -10, 1, 1, 0, 0, 0);
}

void setText(X11Context& ctx, const std::string& text) {
    gOwnedText = text;
    gOwningSelection = true;
    XSetSelectionOwner(ctx.display, ctx.atoms.clipboard,
                       ctx.clipboardOwnerWindow, CurrentTime);
}

std::string getText(X11Context& ctx) {
    Window owner = XGetSelectionOwner(ctx.display, ctx.atoms.clipboard);
    if (owner == None) return {};

    if (owner == ctx.clipboardOwnerWindow) {
        return gOwnedText;
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
    std::string result;
    if (XGetWindowProperty(ctx.display, ctx.clipboardOwnerWindow, property, 0,
                           1 << 20, True, AnyPropertyType, &actualType,
                           &actualFormat, &itemCount, &bytesAfter,
                           &data) == Success &&
        data) {
        result.assign(reinterpret_cast<char*>(data), itemCount);
        XFree(data);
    }
    return result;
}

bool hasText(X11Context& ctx) {
    return XGetSelectionOwner(ctx.display, ctx.atoms.clipboard) != None;
}

void handleSelectionRequest(X11Context& ctx, XSelectionRequestEvent& request) {
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

void handleSelectionClear(X11Context&, XSelectionClearEvent&) {
    gOwningSelection = false;
    gOwnedText.clear();
}

}  // namespace clipboard
