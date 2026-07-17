#pragma once

#include <X11/Xlib.h>
#include <X11/extensions/Xrandr.h>

#include "core/window/WindowTypes.h"
#include "platform/x11/internal/X11Internal.hxx"
#include "platform/x11/window/X11Properties.hxx"

namespace vera::x11::window {

using namespace internal;
using namespace properties;

static void requestNativeRefreshMode(X11Context& ctx) {
    // Best-effort: pick the CRTC's currently-connected output's highest
    // refresh-rate mode at its current resolution. Errors are swallowed --
    // failing to bump refresh rate shouldn't block entering fullscreen.
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
                        mode.height != crtc->height)
                        continue;
                    double refresh =
                        mode.hTotal && mode.vTotal
                            ? mode.dotClock /
                                  (double(mode.hTotal) * mode.vTotal)
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

void apply(X11Context& ctx, Window window, FullScreenMode mode) {
    switch (mode) {
        case FullScreenMode::Windowed:
            setNetWmState(ctx, window, ctx.atoms.netWmStateFullscreen, 0,
                          /*add=*/false);
            break;
        case FullScreenMode::Borderless:
            setNetWmState(ctx, window, ctx.atoms.netWmStateFullscreen, 0,
                          /*add=*/true);
            break;
        case FullScreenMode::Exclusive:
            setNetWmState(ctx, window, ctx.atoms.netWmStateFullscreen, 0,
                          /*add=*/true);
            requestNativeRefreshMode(ctx);
            break;
    }
}

}  // namespace vera::x11::window
