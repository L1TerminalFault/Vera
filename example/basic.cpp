#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

#include "core/app/App.h"
#include "core/app/Types.h"
#include "core_shell/Shell.h"

int main() {
    // -------------------------------------------------------------------------
    // Initialize VeraShell & Perform OS Feature Test Calls
    // -------------------------------------------------------------------------
    VeraShell shell;

    std::cout << "--- Testing VeraShell Linux Services ---" << std::endl;

    // 1. Play System Sound
    std::cout << "[Shell Test] Playing Notification sound..." << std::endl;
    shell.playSound(SystemSound::Notification);

    // 2. Desktop Notification
    NotificationOptions notifOpts{};
    notifOpts.title = "Vera Application Started";
    notifOpts.body = "All Linux shell services initialized successfully.";
    notifOpts.appName = "VeraApp";
    notifOpts.icon = "dialog-information";
    notifOpts.urgency = NotificationUrgency::Normal;

    auto notifResult = shell.showNotification(notifOpts);
    if (notifResult.success) {
        std::cout << "[Shell Test] Notification posted with ID: "
                  << notifResult.notificationId << std::endl;
    }

    // 3. Power Management (Prevent System Sleep)
    SleepRequest sleepReq{};
    sleepReq.target = SleepTarget::System;
    sleepReq.reason = "Vera test suite active instance running";
    if (shell.preventSleep(sleepReq)) {
        std::cout << "[Shell Test] Sleep inhibition activated." << std::endl;
    }

    // 4. System Tray Creation
    TrayOptions trayOpts{};
    trayOpts.id = "vera-test-tray";
    trayOpts.title = "Vera Test Application";
    trayOpts.tooltip = "Vera Instance Monitor";
    trayOpts.iconName = "application-x-executable";
    trayOpts.menuItems = {{"item1", "Open Monitor", true, false, false,
                           []() { std::cout << "Tray: Open clicked\n"; }},
                          {"item2", "Exit", true, false, false,
                           []() { std::cout << "Tray: Exit clicked\n"; }}};

    if (shell.createTray(trayOpts)) {
        std::cout << "[Shell Test] System tray icon registered." << std::endl;
    }

    // 5. App Icon Badge Counter Test
    std::cout << "[Shell Test] Updating application badge count..."
              << std::endl;
    BadgeOptions badgeOpts{};
    badgeOpts.appId = "vera_example";
    badgeOpts.count = 42;
    badgeOpts.countVisible = true;
    if (shell.setBadge(badgeOpts)) {
        std::cout << "[Shell Test] Badge count set to 42." << std::endl;
    }

    // 6. Launcher Dock Progress Bar Test
    std::cout << "[Shell Test] Updating launcher progress status..."
              << std::endl;
    ProgressOptions progOpts{};
    progOpts.appId = "vera_example";
    progOpts.progress = 0.75;
    progOpts.progressVisible = true;
    if (shell.setProgress(progOpts)) {
        std::cout << "[Shell Test] Progress bar set to 75%." << std::endl;
    }

    // 7. Request Application Attention (Urgent Flag)
    std::cout << "[Shell Test] Requesting dock window attention..."
              << std::endl;
    if (shell.requestAttention(AttentionType::Critical, "vera_example")) {
        std::cout << "[Shell Test] Urgent attention signal broadcasted."
                  << std::endl;
    }

    // 8. Message Dialog Interface Test
    std::cout << "[Shell Test] Spawning interactive question dialog..."
              << std::endl;
    DialogOptions diagOpts{};
    diagOpts.title = "Vera Verification Engine";
    diagOpts.message =
        "Do you want to continue initialization of the file picker test "
        "layers?";
    diagOpts.icon = DialogIcon::Question;
    // diagOpts.buttons = {DialogButton::Yes, DialogButton::No};

    DialogResult diagResult = shell.showDialog(diagOpts);
    if (diagResult.button != DialogButton::Yes) {
        std::cout
            << "[Shell Test] User rejected test phase initialization. Exiting."
            << std::endl;
        return 0;
    }
    std::cout << "[Shell Test] User verified dialog action: YES." << std::endl;

    // 9. Open File Dialog Test
    std::cout << "[Shell Test] Spawning Open File Dialog..." << std::endl;
    FileDialogOptions openOpts{};
    openOpts.title = "Select Source Asset Configuration File";
    openOpts.defaultPath =
        std::filesystem::current_path();  // Default to runtime execution folder

    std::filesystem::path chosenOpenPath = shell.openFileDialog(openOpts);
    if (!chosenOpenPath.empty()) {
        std::cout << "[Shell Test] Target file selected for opening: "
                  << chosenOpenPath << std::endl;
    } else {
        std::cout << "[Shell Test] Open File Dialog cancelled by user."
                  << std::endl;
    }
    //
    // // 10. Save File Dialog Test
    // std::cout << "[Shell Test] Spawning Save File Dialog..." << std::endl;
    // SaveFileDialogOptions saveOpts{};
    // saveOpts.title = "Export Render Target Output Matrix";
    // saveOpts.defaultPath = std::filesystem::current_path();
    // saveOpts.defaultName = "untitled_export.json";
    //
    // std::filesystem::path chosenSavePath = shell.saveFileDialog(saveOpts);
    // if (!chosenSavePath.empty()) {
    //     std::cout << "[Shell Test] Target destination designated for saving:
    //     "
    //               << chosenSavePath << std::endl;
    // } else {
    //     std::cout << "[Shell Test] Save File Dialog cancelled by user."
    //               << std::endl;
    // }
    //
    // std::cout << "---------------------------------------" << std::endl
    //           << std::endl;
    //
    // -------------------------------------------------------------------------
    // Application Initialization & Window Loop
    // -------------------------------------------------------------------------
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

        window->setCloseRequestCallback([&app, window, &shell]() -> bool {
            std::cout << "[Instance " << window->getHandle().value
                      << "] Close request approved." << std::endl;

            // Play sound when window closes
            shell.playSound(SystemSound::Success);

            app.destroyWindow(window);
            return true;
        });

        auto windowHandle = window->getHandle().value;

        window->setKeyCallback([windowHandle, &app, &shell](
                                   VeraKey key, bool pressed, bool repeat) {
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

            VeraWindow* win = app.getWindowByHandle(
                static_cast<VeraWindowHandle>(windowHandle));

            if (win) {
                bool isKPressed = (key == VeraKey::KLower)
                                      ? pressed
                                      : win->isPressed(VeraKey::KLower);

                if (isKPressed) {
                    std::cout << " | K Pressed";
                    // Request dock/window attention on pressing K
                    shell.requestAttention(AttentionType::Informational,
                                           "Window requested focus");
                }
            }

            std::cout << std::endl;
        });

        window->setMouseMoveCallback([window](double x, double y) {
            std::cout << "[Instance " << window->getHandle().value
                      << "] Mouse position: " << x << ", " << y << std::endl;
        });
    }

    // Main Event Loop
    while (app.getWindowCount() > 0) {
        app.pollEvents();
    }

    // -------------------------------------------------------------------------
    // Cleanup Shell Services
    // -------------------------------------------------------------------------
    shell.allowSleep(SleepTarget::System);
    shell.removeTray();

    std::cout << "All 3 windows closed cleanly. Vera shutting down."
              << std::endl;
    return 0;
}
