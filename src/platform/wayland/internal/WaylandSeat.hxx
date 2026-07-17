#pragma once

#include <cstring>

#include "platform/wayland/input/WaylandKeyboard.hxx"
#include "platform/wayland/input/WaylandPointer.hxx"
#include "platform/wayland/internal/WaylandInternal.hxx"

namespace vera::wayland::internal {

using namespace input;

static void seatHandleCapabilities(void* data, wl_seat* seat,
                                   uint32_t capabilities) {
    auto* ctx = static_cast<WaylandContext*>(data);

    if ((capabilities & WL_SEAT_CAPABILITY_POINTER) && !ctx->pointer) {
        ctx->pointer = wl_seat_get_pointer(seat);
        addListener(*ctx, ctx->pointer);
    } else if (!(capabilities & WL_SEAT_CAPABILITY_POINTER) && ctx->pointer) {
        wl_pointer_release(ctx->pointer);
        ctx->pointer = nullptr;
    }

    if ((capabilities & WL_SEAT_CAPABILITY_KEYBOARD) && !ctx->keyboard) {
        ctx->keyboard = wl_seat_get_keyboard(seat);
        addListener(*ctx, ctx->keyboard);
    } else if (!(capabilities & WL_SEAT_CAPABILITY_KEYBOARD) && ctx->keyboard) {
        wl_keyboard_release(ctx->keyboard);
        ctx->keyboard = nullptr;
    }
}

static void seatHandleName(void* data, wl_seat* seat, const char* name) {
    (void)data;
    (void)seat;
    (void)name;
#ifdef DEBUG
    std::cout << "[Wayland] Active seat: " << name << "\n";
#endif
}

static const wl_seat_listener kSeatListener = {
    .capabilities = seatHandleCapabilities, .name = seatHandleName};

void bind(WaylandContext& ctx, wl_registry* registry, uint32_t name,
          uint32_t version) {
    ctx.seat = static_cast<wl_seat*>(wl_registry_bind(
        registry, name, &wl_seat_interface, std::min(version, 7u)));

    if (ctx.seat) {
        wl_seat_add_listener(ctx.seat, &kSeatListener, &ctx);
    }
}

void bind(WaylandContext& ctx, wl_registry* registry, uint32_t name,
          uint32_t version);

}  // namespace vera::wayland::internal
