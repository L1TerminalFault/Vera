#include "core/app/App.h"

int main() {
    VeraApp app({});

    auto expectedWindow =
        app.createWindow({.width = 600, .height = 400, .title = "Minimal"});
    if (!expectedWindow.has_value()) {
        printf("Failed to initialize Vera window \n");
        return -1;
    }

    expectedWindow.value()->setCloseRequestCallback([&]() {
        app.destroyWindow(expectedWindow.value());
        return true;
    });

    while (app.getWindowCount()) app.pollEvents();

    printf("Good Bye!\n");
    return 0;
}
