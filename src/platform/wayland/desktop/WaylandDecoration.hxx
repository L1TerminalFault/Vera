#pragma once

#include "platform/wayland/internal/WaylandInternal.hxx"
#include "platform/wayland/internal/protocols/xdg-decoration-unstable-v1-client-protocol.h"

namespace vera::wayland::desktop::decoration {

using namespace internal;

static void decorationHandleConfigure(void* data,
                                      zxdg_toplevel_decoration_v1* decoration,
                                      uint32_t mode) {
    (void)data;
    (void)decoration;
    (void)mode;
#ifdef DEBUG
    if (mode == ZXDG_TOPLEVEL_DECORATION_V1_MODE_SERVER_SIDE) {
        std::cout << "[Wayland] Server-side window decorations activated.\n";
    } else {
        std::cout << "[Wayland] Compositor requested Client-Side Decorations "
                     "(CSD).\n";
    }
#endif
}

static const zxdg_toplevel_decoration_v1_listener kDecorationListener = {
    .configure = decorationHandleConfigure};

void initialize(WaylandContext& ctx) { (void)ctx; }

zxdg_toplevel_decoration_v1* createDecoration(WaylandContext& ctx,
                                              xdg_toplevel* toplevel) {
    if (!ctx.decorationManager || !toplevel) {
        return nullptr;
    }

    zxdg_toplevel_decoration_v1* decoration =
        zxdg_decoration_manager_v1_get_toplevel_decoration(
            ctx.decorationManager, toplevel);

    if (decoration) {
        zxdg_toplevel_decoration_v1_add_listener(decoration,
                                                 &kDecorationListener, nullptr);
        zxdg_toplevel_decoration_v1_set_mode(
            decoration, ZXDG_TOPLEVEL_DECORATION_V1_MODE_SERVER_SIDE);
    }

    return decoration;
}

void destroyDecoration(zxdg_toplevel_decoration_v1* decoration) {
    if (decoration) {
        zxdg_toplevel_decoration_v1_destroy(decoration);
    }
}

}  // namespace vera::wayland::desktop::decoration
