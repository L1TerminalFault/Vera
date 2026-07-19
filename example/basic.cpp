#include <iostream>
#include <string>
#include <vector>

#include "core/app/App.h"

int main() {
    VeraAppInfo appInfo{};
    appInfo.enablePlatformDebugging = true;

    VeraApp app(appInfo);
    VeraSettings settings({{.delayMs = 550, .rate = 5}});
    app.applySettings(settings);

    std::vector<VeraWindow*> activeWindows;
    const int windowCount = 3;

    for (int i = 0; i < windowCount; ++i) {
        VeraWindowInfo winInfo{};
        winInfo.width = 600;
        winInfo.height = 400;

        winInfo.centerOnMonitor = false;
        winInfo.x = 100 + (i * 100);
        winInfo.y = 100 + (i * 100);

        winInfo.title = "Vera Instance #" + std::to_string(i + 1);
        winInfo.customTitleBar = true;
        winInfo.titleBarHeight = 40;

        auto result = app.createWindow(winInfo);
        if (!result.has_value()) {
            std::cerr << "Failed to initialize Vera window index " << i
                      << std::endl;
            return -1;
        }

        VeraWindow* window = result.value();
        activeWindows.push_back(window);

        VeraHitTestRegions regions{};
        regions.dragRegion = VeraRect{0, 0, 600, 40};
        regions.minimizeButton = VeraRect{600 - 135, 0, 45, 40};
        regions.maximizeButton = VeraRect{600 - 90, 0, 45, 40};
        regions.closeButton = VeraRect{600 - 45, 0, 45, 40};
        window->setTitlebarHitTestRegions(regions);

        window->setResizeCallback([window](uint32_t w, uint32_t h) {
            std::cout << "[Instance " << window->getHandle().value
                      << "] Resized to: " << w << "x" << h << std::endl;

            VeraHitTestRegions dynamicRegions{};
            dynamicRegions.dragRegion = VeraRect{0, 0, w, 40};
            dynamicRegions.minimizeButton = VeraRect{w - 135, 0, 45, 40};
            dynamicRegions.maximizeButton = VeraRect{w - 90, 0, 45, 40};
            dynamicRegions.closeButton = VeraRect{w - 45, 0, 45, 40};
            window->setTitlebarHitTestRegions(dynamicRegions);
        });

        window->setCloseRequestCallback([&app, window]() -> bool {
            std::cout << "[Instance " << window->getHandle().value
                      << "] Close request approved." << std::endl;
            app.destroyWindow(window);
            return true;
        });

        auto windowHandle = window->getHandle().value;

        window->setKeyCallback([windowHandle](VeraKey key, bool pressed,
                                              bool repeat) {
            std::cout << "[Instance " << windowHandle << "] ";

            if (pressed && repeat) {
                std::cout << "Key Held Enum ID: " << static_cast<uint16_t>(key);
            } else if (pressed) {
                std::cout << "Key Pressed Enum ID: "
                          << static_cast<uint16_t>(key);
            } else {
                std::cout << "Key Released Enum ID: "
                          << static_cast<uint16_t>(key);
            }

            std::cout << std::endl;
        });

        window->setMouseMoveCallback([window](double x, double y) {
            std::cout << "[Instance " << window->getHandle().value
                      << "] Mouse position: " << x << ", " << y << std::endl;
        });
    }

    while (app.getWindowCount() > 0) {
        app.pollEvents();
    }

    std::cout << "All 3 windows closed cleanly. Vera shutting down."
              << std::endl;
    return 0;
}
