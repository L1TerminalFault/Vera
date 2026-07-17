#pragma once

#include <sys/mman.h>
#include <unistd.h>
#include <xkbcommon/xkbcommon.h>

#include "platform/wayland/internal/WaylandInternal.hxx"
#include "platform/wayland/window/WaylandWindow.hxx"

namespace vera::wayland::input {

using namespace internal;

static VeraKey translateKeysym(xkb_keysym_t sym) {
    switch (sym) {
        case XKB_KEY_Escape:
            return VeraKey::Escape;
        case XKB_KEY_Return:
            return VeraKey::Enter;
        case XKB_KEY_space:
            return VeraKey::Space;
        case XKB_KEY_BackSpace:
            return VeraKey::Backspace;
        case XKB_KEY_Tab:
            return VeraKey::Tab;

        case XKB_KEY_a:
        case XKB_KEY_A:
            return VeraKey::A;
        case XKB_KEY_b:
        case XKB_KEY_B:
            return VeraKey::B;
        case XKB_KEY_c:
        case XKB_KEY_C:
            return VeraKey::C;
        case XKB_KEY_d:
        case XKB_KEY_D:
            return VeraKey::D;
        case XKB_KEY_e:
        case XKB_KEY_E:
            return VeraKey::E;
        case XKB_KEY_f:
        case XKB_KEY_F:
            return VeraKey::F;
        case XKB_KEY_g:
        case XKB_KEY_G:
            return VeraKey::G;
        case XKB_KEY_h:
        case XKB_KEY_H:
            return VeraKey::H;
        case XKB_KEY_i:
        case XKB_KEY_I:
            return VeraKey::I;
        case XKB_KEY_j:
        case XKB_KEY_J:
            return VeraKey::J;
        case XKB_KEY_k:
        case XKB_KEY_K:
            return VeraKey::K;
        case XKB_KEY_l:
        case XKB_KEY_L:
            return VeraKey::L;
        case XKB_KEY_m:
        case XKB_KEY_M:
            return VeraKey::M;
        case XKB_KEY_n:
        case XKB_KEY_N:
            return VeraKey::N;
        case XKB_KEY_o:
        case XKB_KEY_O:
            return VeraKey::O;
        case XKB_KEY_p:
        case XKB_KEY_P:
            return VeraKey::P;
        case XKB_KEY_q:
        case XKB_KEY_Q:
            return VeraKey::Q;
        case XKB_KEY_r:
        case XKB_KEY_R:
            return VeraKey::R;
        case XKB_KEY_s:
        case XKB_KEY_S:
            return VeraKey::S;
        case XKB_KEY_t:
        case XKB_KEY_T:
            return VeraKey::T;
        case XKB_KEY_u:
        case XKB_KEY_U:
            return VeraKey::U;
        case XKB_KEY_v:
        case XKB_KEY_V:
            return VeraKey::V;
        case XKB_KEY_w:
        case XKB_KEY_W:
            return VeraKey::W;
        case XKB_KEY_x:
        case XKB_KEY_X:
            return VeraKey::X;
        case XKB_KEY_y:
        case XKB_KEY_Y:
            return VeraKey::Y;
        case XKB_KEY_z:
        case XKB_KEY_Z:
            return VeraKey::Z;

        case XKB_KEY_Left:
            return VeraKey::Left;
        case XKB_KEY_Right:
            return VeraKey::Right;
        case XKB_KEY_Up:
            return VeraKey::Up;
        case XKB_KEY_Down:
            return VeraKey::Down;

        default:
            return VeraKey::Unknown;
    }
}

static void keyboardHandleKeymap(void* data, wl_keyboard* keyboard,
                                 uint32_t format, int32_t fd, uint32_t size) {
    auto* ctx = static_cast<WaylandContext*>(data);
    (void)keyboard;

    if (format != WL_KEYBOARD_KEYMAP_FORMAT_XKB_V1) {
        close(fd);
        return;
    }

    char* mapStr =
        static_cast<char*>(mmap(nullptr, size, PROT_READ, MAP_SHARED, fd, 0));
    if (mapStr == MAP_FAILED) {
        close(fd);
        return;
    }

    if (!ctx->xkbContext) {
        ctx->xkbContext = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
    }

    if (ctx->xkbKeymap) xkb_keymap_unref(ctx->xkbKeymap);
    ctx->xkbKeymap = xkb_keymap_new_from_string(ctx->xkbContext, mapStr,
                                                XKB_KEYMAP_FORMAT_TEXT_V1,
                                                XKB_KEYMAP_COMPILE_NO_FLAGS);
    munmap(mapStr, size);
    close(fd);

    if (ctx->xkbKeymap) {
        if (ctx->xkbState) xkb_state_unref(ctx->xkbState);
        ctx->xkbState = xkb_state_new(ctx->xkbKeymap);
    }
}

static void keyboardHandleEnter(void* data, wl_keyboard* keyboard,
                                uint32_t serial, wl_surface* surface,
                                wl_array* keys) {
    auto* ctx = static_cast<WaylandContext*>(data);
    (void)keyboard;
    (void)serial;
    (void)keys;

    auto it = ctx->windowsBySurface.find(surface);
    if (it != ctx->windowsBySurface.end()) {
        ctx->focusedWindow = it->second;
    }
}

static void keyboardHandleLeave(void* data, wl_keyboard* keyboard,
                                uint32_t serial, wl_surface* surface) {
    auto* ctx = static_cast<WaylandContext*>(data);
    (void)keyboard;
    (void)serial;
    (void)surface;
    ctx->focusedWindow = nullptr;
}

static void keyboardHandleKey(void* data, wl_keyboard* keyboard,
                              uint32_t serial, uint32_t time, uint32_t key,
                              uint32_t state) {
    auto* ctx = static_cast<WaylandContext*>(data);
    (void)keyboard;
    (void)serial;
    (void)time;

    if (!ctx->focusedWindow || !ctx->xkbState) return;

    auto* win = ctx->focusedWindow;
    uint32_t scanCode = key + 8;
    xkb_keysym_t sym = xkb_state_key_get_one_sym(ctx->xkbState, scanCode);

    VeraKey veraKey = translateKeysym(sym);
    bool pressed = (state == WL_KEYBOARD_KEY_STATE_PRESSED);

    if (win->getKeyCallback()) {
        win->getKeyCallback()(veraKey, pressed, false);
    }

    if (pressed) {
        uint32_t utf32 = xkb_state_key_get_utf32(ctx->xkbState, scanCode);
        if (utf32 > 0 && win->getCharCallback()) {
            win->getCharCallback()(utf32);
        }
    }
}

static void keyboardHandleModifiers(void* data, wl_keyboard* keyboard,
                                    uint32_t serial, uint32_t mods_depressed,
                                    uint32_t mods_latched, uint32_t mods_locked,
                                    uint32_t group) {
    auto* ctx = static_cast<WaylandContext*>(data);
    (void)keyboard;
    (void)serial;

    if (ctx->xkbState) {
        xkb_state_update_mask(ctx->xkbState, mods_depressed, mods_latched,
                              mods_locked, 0, 0, group);
    }
}

static void keyboardHandleRepeatInfo(void* data, wl_keyboard* keyboard,
                                     int32_t rate, int32_t delay) {
    (void)data;
    (void)keyboard;
    (void)rate;
    (void)delay;
}

static const wl_keyboard_listener kKeyboardListener = {
    .keymap = keyboardHandleKeymap,
    .enter = keyboardHandleEnter,
    .leave = keyboardHandleLeave,
    .key = keyboardHandleKey,
    .modifiers = keyboardHandleModifiers,
    .repeat_info = keyboardHandleRepeatInfo};

void addListener(WaylandContext& ctx, wl_keyboard* keyboard) {
    if (keyboard) {
        wl_keyboard_add_listener(keyboard, &kKeyboardListener, &ctx);
    }
}

}  // namespace vera::wayland::input
