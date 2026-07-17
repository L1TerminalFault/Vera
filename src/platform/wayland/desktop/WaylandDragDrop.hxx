#pragma once

#include <fcntl.h>
#include <poll.h>
#include <unistd.h>

#include <cstring>
#include <functional>
#include <sstream>
#include <string>
#include <vector>

#include "core/window/DragDrop.h"
#include "platform/wayland/internal/WaylandInternal.hxx"
#include "platform/wayland/internal/protocols/xdg-shell-client-protocol.h"

namespace vera::wayland::desktop::dnd {

using namespace core::window;
using namespace internal;

static VeraDragCallback g_DragCallback = nullptr;
static wl_data_offer* g_ActiveDndOffer = nullptr;
static double g_LastDragX = 0.0;
static double g_LastDragY = 0.0;

static std::string urlDecode(const std::string& SRC) {
    std::string ret;
    char ch;
    int /* i, */ ii;
    for (size_t pos = 0; pos < SRC.length(); ++pos) {
        if (SRC[pos] == '%') {
            sscanf(SRC.substr(pos + 1, 2).c_str(), "%x", &ii);
            ch = static_cast<char>(ii);
            ret += ch;
            pos += 2;
        } else if (SRC[pos] == '+') {
            ret += ' ';
        } else {
            ret += SRC[pos];
        }
    }
    return ret;
}

static std::vector<std::string> parseUriList(const std::string& rawData) {
    std::vector<std::string> paths;
    std::stringstream ss(rawData);
    std::string line;

    while (std::getline(ss, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        if (line.empty() || line[0] == '#') {
            continue;
        }

        if (line.rfind("file://", 0) == 0) {
            std::string decodedPath = urlDecode(line.substr(7));
            paths.push_back(decodedPath);
        }
    }
    return paths;
}

static void handleDragEnter(void* data, wl_data_device* device, uint32_t serial,
                            wl_surface* surface, wl_fixed_t x, wl_fixed_t y,
                            wl_data_offer* offer) {
    (void)data;
    (void)device;
    (void)serial;
    (void)surface;

    g_ActiveDndOffer = offer;
    g_LastDragX = wl_fixed_to_double(x);
    g_LastDragY = wl_fixed_to_double(y);

    if (g_ActiveDndOffer) {
        wl_data_offer_accept(g_ActiveDndOffer, serial, "text/uri-list");
    }
}

static void handleDragMotion(void* data, wl_data_device* device, uint32_t time,
                             wl_fixed_t x, wl_fixed_t y) {
    (void)data;
    (void)device;
    (void)time;

    g_LastDragX = wl_fixed_to_double(x);
    g_LastDragY = wl_fixed_to_double(y);
}

static void handleDragLeave(void* data, wl_data_device* device) {
    (void)data;
    (void)device;
    g_ActiveDndOffer = nullptr;
}

static void handleDragDrop(void* data, wl_data_device* device) {
    auto* ctx = static_cast<WaylandContext*>(data);
    (void)device;

    if (!g_ActiveDndOffer || !g_DragCallback) {
        return;
    }

    int fds[2];
    if (pipe2(fds, O_CLOEXEC | O_NONBLOCK) < 0) {
        return;
    }

    wl_data_offer_receive(g_ActiveDndOffer, "text/uri-list", fds[1]);
    close(fds[1]);

    wl_display_flush(ctx->display);
    wl_display_roundtrip(ctx->display);

    std::string rawPayload = "";
    char buffer[1024];
    struct pollfd pfd = {.fd = fds[0], .events = POLLIN, .revents = 0};

    while (poll(&pfd, 1, 100) > 0) {
        ssize_t bytesRead = read(fds[0], buffer, sizeof(buffer));
        if (bytesRead <= 0) {
            break;
        }
        rawPayload.append(buffer, bytesRead);
    }
    close(fds[0]);

    std::vector<std::string> paths = parseUriList(rawPayload);
    if (!paths.empty()) {
        VeraDragEvent event{};
        event.paths = std::move(paths);
        event.x = g_LastDragX;
        event.y = g_LastDragY;

        g_DragCallback(event);
    }

    wl_data_offer_finish(g_ActiveDndOffer);
    g_ActiveDndOffer = nullptr;
}

static const wl_data_device_listener kDataDeviceDndListener = {
    .data_offer = [](void*, wl_data_device*,
                     wl_data_offer*) { /* Handled contextually */ },
    .enter = handleDragEnter,
    .leave = handleDragLeave,
    .motion = handleDragMotion,
    .drop = handleDragDrop,
    .selection = [](void*, wl_data_device*,
                    wl_data_offer*) { /* Exclusively for Clipboard */ }};

void initialize(WaylandContext& ctx) {
    if (ctx.dataDevice) {
        wl_data_device_add_listener(ctx.dataDevice, &kDataDeviceDndListener,
                                    &ctx);
    }
}

void setDragCallback(WaylandContext& ctx, VeraDragCallback callback) {
    (void)ctx;
    g_DragCallback = std::move(callback);
}

void shutdown() {
    g_DragCallback = nullptr;
    g_ActiveDndOffer = nullptr;
}

}  // namespace vera::wayland::desktop::dnd
