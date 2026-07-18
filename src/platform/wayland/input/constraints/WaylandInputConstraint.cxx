#include "WaylandInputConstraint.hxx"

#include "platform/wayland/internal/protocols/pointer-constraints-unstable-v1-client-protocol.h"
#include "platform/wayland/internal/protocols/relative-pointer-unstable-v1-client-protocol.h"
#include "platform/wayland/window/WaylandWindow.hxx"

inline static zwp_locked_pointer_v1* sLockedPointer = nullptr;
inline static zwp_relative_pointer_v1* sRelativePointer = nullptr;

static void relativePointerHandleMotion(
    void* data, zwp_relative_pointer_v1* relativePointer, uint32_t time_hi,
    uint32_t time_lo, wl_fixed_t dx, wl_fixed_t dy, wl_fixed_t dx_unaccel,
    wl_fixed_t dy_unaccel) {
    auto* ctx = static_cast<WaylandContext*>(data);
    (void)relativePointer;
    (void)time_hi;
    (void)time_lo;
    (void)dx;
    (void)dy;

    double rawDeltaX = wl_fixed_to_double(dx_unaccel);
    double rawDeltaY = wl_fixed_to_double(dy_unaccel);

    if (ctx->focusedWindow && ctx->focusedWindow->getMouseMoveCallback()) {
        ctx->focusedWindow->getMouseMoveCallback()(rawDeltaX, rawDeltaY);
    }
}

static const zwp_relative_pointer_v1_listener KRELATIVE_POINTER_LISTENER = {
    .relative_motion = relativePointerHandleMotion};

static void lockedPointerHandleLocked(void* data,
                                      zwp_locked_pointer_v1* lockedPointer) {
    (void)data;
    (void)lockedPointer;
#ifdef DEBUG
    std::cout << "[Wayland] Pointer constraint lock acquired.\n";
#endif
}

static void lockedPointerHandleUnlocked(void* data,
                                        zwp_locked_pointer_v1* lockedPointer) {
    (void)data;
    (void)lockedPointer;
#ifdef DEBUG
    std::cout << "[Wayland] Pointer constraint lock released.\n";
#endif
}

static const zwp_locked_pointer_v1_listener KLOCKED_POINTER_LISTENER = {
    .locked = lockedPointerHandleLocked,
    .unlocked = lockedPointerHandleUnlocked};

void unlockPointerWayland(WaylandContext& ctx) {
    (void)ctx;

    if (sRelativePointer) {
        zwp_relative_pointer_v1_destroy(sRelativePointer);
        sRelativePointer = nullptr;
    }

    if (sLockedPointer) {
        zwp_locked_pointer_v1_destroy(sLockedPointer);
        sLockedPointer = nullptr;
    }
}

void lockPointerWayland(WaylandContext& ctx, wl_surface* surface) {
    if (!ctx.pointerConstraints || !ctx.relativePointerManager ||
        !ctx.pointer || !surface) {
        return;
    }

    unlockPointerWayland(ctx);

    sLockedPointer = zwp_pointer_constraints_v1_lock_pointer(
        ctx.pointerConstraints, surface, ctx.pointer, nullptr,
        ZWP_POINTER_CONSTRAINTS_V1_LIFETIME_PERSISTENT);

    if (sLockedPointer) {
        zwp_locked_pointer_v1_add_listener(sLockedPointer,
                                           &KLOCKED_POINTER_LISTENER, &ctx);
    }

    sRelativePointer = zwp_relative_pointer_manager_v1_get_relative_pointer(
        ctx.relativePointerManager, ctx.pointer);

    if (sRelativePointer) {
        zwp_relative_pointer_v1_add_listener(sRelativePointer,
                                             &KRELATIVE_POINTER_LISTENER, &ctx);
    }
}
