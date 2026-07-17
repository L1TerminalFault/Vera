#pragma once
// Shared, backend-private state. Nothing outside platform/x11/ should ever
// include this header -- it is glue, not API surface.
#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include <cstdint>
#include <unordered_map>

#include "core/window/WindowTypes.h"

namespace vera::x11::window {
class X11Window;
}

namespace vera::x11::internal {

using namespace window;

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

struct X11Context {
    Display* display = nullptr;
    int screen = 0;
    Window root = 0;
    X11Atoms atoms;

    // The global Input Method resource
    XIM xim = nullptr;

    // XID -> owning VeraWindow
    std::unordered_map<::Window, X11Window*> windowsByXid;

    // Clipboard helper
    Window clipboardOwnerWindow = 0;

    uint64_t nextHandleValue = 1;

    VeraWindowHandle allocateHandle() {
        return VeraWindowHandle{nextHandleValue++};
    }
};

}  // namespace vera::x11::internal
