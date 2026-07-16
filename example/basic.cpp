#include <iostream>
#include <string>
#include <vector>

#include "core/app/app.h"

int main() {
    VeraAppInfo app_info{};
    app_info.enablePlatformDebugging = true;

    VeraApp app(app_info);

    std::vector<VeraWindow*> active_windows;
    const int window_count = 3;

    for (int i = 0; i < window_count; ++i) {
        VeraWindowInfo win_info{};
        win_info.width = 600;
        win_info.height = 400;

        win_info.centerOnMonitor = false;
        win_info.x = 100 + (i * 100);
        win_info.y = 100 + (i * 100);

        win_info.title = "Vera Instance #" + std::to_string(i + 1);
        win_info.customTitleBar = true;
        win_info.titleBarHeight = 40;

        auto result = app.createWindow(win_info);
        if (!result.has_value()) {
            std::cerr << "Failed to initialize Vera window index " << i
                      << std::endl;
            return -1;
        }

        VeraWindow* window = result.value();
        active_windows.push_back(window);

        VeraHitTestRegions regions{};
        regions.dragRegion = VeraRect{0, 0, 600, 40};
        regions.minimizeButton = VeraRect{600 - 135, 0, 45, 40};
        regions.maximizeButton =
            VeraRect{600 - 90, 0, 45, 40}; 
        regions.closeButton = VeraRect{600 - 45, 0, 45, 40};
        window->setTitlebarHitTestRegions(regions);

        window->setResizeCallback([window](uint32_t w, uint32_t h) {
            std::cout << "[Instance " << window->getHandle().value
                      << "] Resized to: " << w << "x" << h << std::endl;

            VeraHitTestRegions dynamic_regions{};
            dynamic_regions.dragRegion = VeraRect{0, 0, w, 40};
            dynamic_regions.minimizeButton = VeraRect{w - 135, 0, 45, 40};
            dynamic_regions.maximizeButton = VeraRect{w - 90, 0, 45, 40};
            dynamic_regions.closeButton = VeraRect{w - 45, 0, 45, 40};
            window->setTitlebarHitTestRegions(dynamic_regions);
        });

        window->setCloseRequestCallback([&app, window]() -> bool {
            std::cout << "[Instance " << window->getHandle().value
                      << "] Close request approved." << std::endl;
            return true;
        });

        window->setKeyCallback(
            [window](VeraKey key, bool pressed, bool repeat) {
                if (pressed && !repeat) {
                    std::cout << "[Instance " << window->getHandle().value
                              << "] Received Key Press Enum ID: "
                              << static_cast<uint16_t>(key) << std::endl;
                }
            });
    }


    while (app.getWindowCount() > 0) {
        app.pollEvents();
    }

    std::cout << "All 3 windows closed cleanly. Vera shutting down."
              << std::endl;
    return 0;
}
