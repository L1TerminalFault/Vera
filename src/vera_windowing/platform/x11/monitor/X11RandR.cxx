#include "X11RandR.hxx"

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

std::vector<VeraMonitorInfo> queryMonitorsX11(X11Context& ctx) {
    std::vector<VeraMonitorInfo> out;

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

    for (int i = 0; i < monitorCount; ++i) {
        const XRRMonitorInfo& m = monitors[i];
        char* atomName = XGetAtomName(ctx.display, m.name);

        VeraMonitorInfo info;
        info.name = atomName ? atomName : ("monitor-" + std::to_string(i));
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

std::vector<VeraDisplayModeInfo> queryDisplayModesX11(
    X11Context& ctx, const VeraMonitorInfo& monitor) {
    std::vector<VeraDisplayModeInfo> out;

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
