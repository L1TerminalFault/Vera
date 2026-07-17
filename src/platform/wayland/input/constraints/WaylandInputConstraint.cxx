#include "platform/wayland/input/constraints/WaylandInputConstraint.hxx"

#include "platform/wayland/internal/protocols/pointer-constraints-unstable-v1-client-protocol.h"
#include "platform/wayland/internal/protocols/relative-pointer-unstable-v1-client-protocol.h"
#include "platform/wayland/window/WaylandWindow.hxx"

namespace vera::wayland::inputconstraints {

using namespace internal;

inline static zwp_locked_pointer_v1* s_lockedPointer = nullptr;
inline static zwp_relative_pointer_v1* s_relativePointer = nullptr;

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

static const zwp_relative_pointer_v1_listener kRelativePointerListener = {
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

static const zwp_locked_pointer_v1_listener kLockedPointerListener = {
    .locked = lockedPointerHandleLocked,
    .unlocked = lockedPointerHandleUnlocked};

void unlockPointer(WaylandContext& ctx) {
    (void)ctx;

    if (s_relativePointer) {
        zwp_relative_pointer_v1_destroy(s_relativePointer);
        s_relativePointer = nullptr;
    }

    if (s_lockedPointer) {
        zwp_locked_pointer_v1_destroy(s_lockedPointer);
        s_lockedPointer = nullptr;
    }
}

void lockPointer(WaylandContext& ctx, wl_surface* surface) {
    if (!ctx.pointerConstraints || !ctx.relativePointerManager ||
        !ctx.pointer || !surface) {
        return;
    }

    unlockPointer(ctx);

    s_lockedPointer = zwp_pointer_constraints_v1_lock_pointer(
        ctx.pointerConstraints, surface, ctx.pointer, nullptr,
        ZWP_POINTER_CONSTRAINTS_V1_LIFETIME_PERSISTENT);

    if (s_lockedPointer) {
        zwp_locked_pointer_v1_add_listener(s_lockedPointer,
                                           &kLockedPointerListener, &ctx);
    }

    s_relativePointer = zwp_relative_pointer_manager_v1_get_relative_pointer(
        ctx.relativePointerManager, ctx.pointer);

    if (s_relativePointer) {
        zwp_relative_pointer_v1_add_listener(s_relativePointer,
                                             &kRelativePointerListener, &ctx);
    }
}

}  // namespace vera::wayland::inputconstraints
