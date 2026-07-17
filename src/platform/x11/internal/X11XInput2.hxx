#pragma once
#include <X11/extensions/XInput2.h>

#include <string>
#include <vector>

#include "core/input/Mouse.h"
#include "platform/x11/internal/X11Internal.hxx"

namespace vera::x11::internal::xinput {

using namespace core::input;

bool initialize(X11Context& ctx, int& outOpcode) {
    int event, error;
    if (!XQueryExtension(ctx.display, "XInputExtension", &outOpcode, &event,
                         &error)) {
        return false;
    }
    int major = 2, minor = 0;
    if (XIQueryVersion(ctx.display, &major, &minor) == BadRequest) {
        return false;
    }

    // Subscribe to hierarchy changes (device add/remove/hotplug) on the root
    // window so getInputDevices() stays current without polling.
    XIEventMask mask;
    unsigned char maskBits[XIMaskLen(XI_HierarchyChanged)] = {0};
    mask.deviceid = XIAllDevices;
    mask.mask_len = sizeof(maskBits);
    mask.mask = maskBits;
    XISetMask(maskBits, XI_HierarchyChanged);
    XISelectEvents(ctx.display, ctx.root, &mask, 1);

    return true;
}

std::vector<VeraInputDeviceInfo> enumerateDevices(X11Context& ctx) {
    std::vector<VeraInputDeviceInfo> devices;
    int count = 0;
    XIDeviceInfo* info = XIQueryDevice(ctx.display, XIAllDevices, &count);
    if (!info) return devices;

    for (int i = 0; i < count; ++i) {
        // Only surface master devices (the logical "seat" pointer/keyboard);
        // slave devices are the physical hardware feeding them and are
        // usually noise for an application-level device list.
        if (info[i].use != XIMasterPointer && info[i].use != XIMasterKeyboard) {
            continue;
        }
        devices.push_back(VeraInputDeviceInfo{
            .name = info[i].name ? info[i].name : "Unknown device",
            .connected = info[i].enabled != 0,
        });
    }

    XIFreeDeviceInfo(info);
    return devices;
}

}  // namespace vera::x11::internal::xinput
