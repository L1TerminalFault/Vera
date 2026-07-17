#pragma once

#include "platform/x11/internal/X11Internal.hxx"

namespace vera::x11::internal {

static Atom intern(Display* d, const char* name) {
    return XInternAtom(d, name, False);
}

void internAll(X11Context& ctx) {
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

}  // namespace vera::x11::internal
