#pragma once

#include <fcntl.h>
#include <poll.h>
#include <unistd.h>

#include <cstring>
#include <string>

#include "platform/wayland/internal/WaylandInternal.hxx"

namespace vera::wayland::desktop::clipboard {

using namespace internal;

static std::string g_ClipboardDataStore = "";

static const char* kTextMimeTypes[] = {"text/plain;charset=utf-8", "text/plain",
                                       "UTF8_STRING", "STRING"};
constexpr size_t kMimeTypeCount =
    sizeof(kTextMimeTypes) / sizeof(kTextMimeTypes[0]);

static void handleSourceTarget(void* data, wl_data_source* source,
                               const char* mime_type) {
    (void)data;
    (void)source;
    (void)mime_type;
}

static void handleSourceSend(void* data, wl_data_source* source,
                             const char* mime_type, int32_t fd) {
    (void)data;
    (void)source;

    bool typeMatches = false;
    for (size_t i = 0; i < kMimeTypeCount; ++i) {
        if (mime_type && std::strcmp(mime_type, kTextMimeTypes[i]) == 0) {
            typeMatches = true;
            break;
        }
    }

    if (typeMatches && fd >= 0) {
        ssize_t written = write(fd, g_ClipboardDataStore.c_str(),
                                g_ClipboardDataStore.length());
        (void)written;
    }
    close(fd);
}

static void handleSourceCancelled(void* data, wl_data_source* source) {
    (void)data;
    wl_data_source_destroy(source);
}

static const wl_data_source_listener kDataSourceListener = {
    .target = handleSourceTarget,
    .send = handleSourceSend,
    .cancelled = handleSourceCancelled,
    .dnd_drop_performed = [](void*, wl_data_source*) {},
    .dnd_finished = [](void*, wl_data_source*) {},
    .action = [](void*, wl_data_source*, uint32_t) {},
};

void initialize(WaylandContext& ctx) { (void)ctx; }

std::string getClipboardText(WaylandContext& ctx) {
    if (!ctx.activeClipboardOffer) {
        return "";
    }

    const char* matchedMimeType = "text/plain;charset=utf-8";

    int fds[2];
    if (pipe2(fds, O_CLOEXEC | O_NONBLOCK) < 0) {
        return "";
    }

    wl_data_offer_receive(ctx.activeClipboardOffer, matchedMimeType, fds[1]);
    close(fds[1]);

    wl_display_flush(ctx.display);
    wl_display_roundtrip(ctx.display);

    std::string result = "";
    char buffer[1024];
    struct pollfd pfd = {.fd = fds[0], .events = POLLIN, .revents = 0};

    while (poll(&pfd, 1, 100) > 0) {
        ssize_t bytesRead = read(fds[0], buffer, sizeof(buffer));
        if (bytesRead <= 0) {
            break;
        }
        result.append(buffer, bytesRead);
    }

    close(fds[0]);
    return result;
}

void setClipboardText(WaylandContext& ctx, const std::string& text) {
    if (!ctx.dataDeviceManager || !ctx.dataDevice) {
        return;
    }

    g_ClipboardDataStore = text;

    wl_data_source* source =
        wl_data_device_manager_create_data_source(ctx.dataDeviceManager);
    if (!source) {
        return;
    }

    for (size_t i = 0; i < kMimeTypeCount; ++i) {
        wl_data_source_offer(source, kTextMimeTypes[i]);
    }

    wl_data_source_add_listener(source, &kDataSourceListener, &ctx);

    wl_data_device_set_selection(ctx.dataDevice, source,
                                 ctx.lastPointerButtonSerial);
}

bool hasClipboardText(const WaylandContext& ctx) {
    return ctx.activeClipboardOffer != nullptr;
}

}  // namespace vera::wayland::desktop::clipboard
