#pragma once

#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include <chrono>
#include <cstdint>
#include <unordered_map>

#include "core/app/Types.h"
#include "core/input/Keys.h"

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

struct KeyRepeatStateX11 {
    Window window;
    uint32_t key;
    uint32_t scanCode;
    VeraKey veraKey;
    std::chrono::steady_clock::time_point nextRepeat;
};

struct X11Context {
    Display* display = nullptr;
    int screen = 0;
    Window root = 0;
    X11Atoms atoms;

    XIM xim = nullptr;

    std::unordered_map<::Window, X11Window*> windowsByXid;

    uint32_t keyRepeatRate = 0;
    uint32_t keyRepeatDelay = 0;

    std::unordered_map<uint32_t, KeyRepeatStateX11> pressedKeys;

    Window clipboardOwnerWindow = 0;

    uint64_t nextHandleValue = 1;

    VeraWindowHandle allocateHandle() {
        return VeraWindowHandle{nextHandleValue++};
    }
};
