#include "WaylandDragDrop.hxx"

#include <fcntl.h>
#include <poll.h>
#include <unistd.h>

#include <cstring>
#include <sstream>
#include <string>
#include <vector>

#include "platform/wayland/internal/protocols/xdg-shell-client-protocol.h"

static VeraDragCallback gDragCallback = nullptr;
static wl_data_offer* gActiveDndOffer = nullptr;
static double gLastDragX = 0.0;
static double gLastDragY = 0.0;

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

    gActiveDndOffer = offer;
    gLastDragX = wl_fixed_to_double(x);
    gLastDragY = wl_fixed_to_double(y);

    if (gActiveDndOffer) {
        wl_data_offer_accept(gActiveDndOffer, serial, "text/uri-list");
    }
}

static void handleDragMotion(void* data, wl_data_device* device, uint32_t time,
                             wl_fixed_t x, wl_fixed_t y) {
    (void)data;
    (void)device;
    (void)time;

    gLastDragX = wl_fixed_to_double(x);
    gLastDragY = wl_fixed_to_double(y);
}

static void handleDragLeave(void* data, wl_data_device* device) {
    (void)data;
    (void)device;
    gActiveDndOffer = nullptr;
}

static void handleDragDrop(void* data, wl_data_device* device) {
    auto* ctx = static_cast<WaylandContext*>(data);
    (void)device;

    if (!gActiveDndOffer || !gDragCallback) {
        return;
    }

    int fds[2];
    if (pipe2(fds, O_CLOEXEC | O_NONBLOCK) < 0) {
        return;
    }

    wl_data_offer_receive(gActiveDndOffer, "text/uri-list", fds[1]);
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
        event.x = gLastDragX;
        event.y = gLastDragY;

        gDragCallback(event);
    }

    wl_data_offer_finish(gActiveDndOffer);
    gActiveDndOffer = nullptr;
}

static const wl_data_device_listener KDATA_DEVICE_DND_LISTENER = {
    .data_offer = [](void*, wl_data_device*,
                     wl_data_offer*) { /* Handled contextually */ },
    .enter = handleDragEnter,
    .leave = handleDragLeave,
    .motion = handleDragMotion,
    .drop = handleDragDrop,
    .selection = [](void*, wl_data_device*,
                    wl_data_offer*) { /* Exclusively for Clipboard */ }};

void initializeDnDWayland(WaylandContext& ctx) {
    if (ctx.dataDevice) {
        wl_data_device_add_listener(ctx.dataDevice, &KDATA_DEVICE_DND_LISTENER,
                                    &ctx);
    }
}

void setDragCallbackWayland(WaylandContext& ctx, VeraDragCallback callback) {
    (void)ctx;
    gDragCallback = std::move(callback);
}

void shutdownDnDWayland() {
    gDragCallback = nullptr;
    gActiveDndOffer = nullptr;
}
