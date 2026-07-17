#include "X11Joystick.hxx"

#include <dirent.h>
#include <fcntl.h>
#include <linux/joystick.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include <cstdint>
#include <functional>
#include <iostream>
#include <string>
#include <vector>

namespace vera::x11::input {

using namespace core::input;
using namespace internal;

struct JoystickDevice {
    int fd = -1;
    std::string devicePath;
    VeraJoystickState state;
};

// Static storage for joystick devices and callbacks
static std::vector<JoystickDevice> g_Joysticks(16);
static std::function<void(uint32_t, uint32_t, bool)> g_ButtonCallback;
static std::function<void(uint32_t, uint32_t, float)> g_AxisCallback;

void setButtonCallback(std::function<void(uint32_t, uint32_t, bool)> cb) {
    g_ButtonCallback = cb;
}
void setAxisCallback(std::function<void(uint32_t, uint32_t, float)> cb) {
    g_AxisCallback = cb;
}

void initialize(X11Context& ctx) {
    (void)ctx;

    DIR* devDir = opendir("/dev/input");
    if (!devDir) {
        std::cerr << "[Joystick] Failed to open /dev/input directory.\n";
        return;
    }

    struct dirent* entry;
    uint32_t slot = 0;

    while ((entry = readdir(devDir)) != nullptr && slot < g_Joysticks.size()) {
        std::string name(entry->d_name);
        if (name.rfind("js", 0) == 0) {
            std::string path = "/dev/input/" + name;
            int fd = open(path.c_str(), O_RDONLY | O_NONBLOCK);
            if (fd >= 0) {
                char devName[128] = "Unknown Controller";
                ioctl(fd, JSIOCGNAME(sizeof(devName)), devName);

                uint8_t axesCount = 0;
                uint8_t buttonsCount = 0;
                ioctl(fd, JSIOCGAXES, &axesCount);
                ioctl(fd, JSIOCGBUTTONS, &buttonsCount);

                JoystickDevice& joy = g_Joysticks[slot];
                joy.fd = fd;
                joy.devicePath = path;
                joy.state.name = devName;
                joy.state.connected = true;
                joy.state.axes.resize(axesCount, 0.0f);
                joy.state.buttons.resize(buttonsCount, false);

                std::cout << "[Joystick] Registered [" << slot
                          << "]: " << joy.state.name << "\n";
                slot++;
            }
        }
    }
    closedir(devDir);
}

void update(X11Context& ctx) {
    (void)ctx;

    for (size_t i = 0; i < g_Joysticks.size(); ++i) {
        JoystickDevice& joy = g_Joysticks[i];
        if (joy.fd < 0) continue;

        struct js_event event;
        while (read(joy.fd, &event, sizeof(event)) > 0) {
            uint8_t type = event.type & ~JS_EVENT_INIT;

            if (type == JS_EVENT_BUTTON) {
                if (event.number < joy.state.buttons.size()) {
                    joy.state.buttons[event.number] = (event.value != 0);
                    if (g_ButtonCallback) {
                        g_ButtonCallback(static_cast<uint32_t>(i), event.number,
                                         joy.state.buttons[event.number]);
                    }
                }
            } else if (type == JS_EVENT_AXIS) {
                if (event.number < joy.state.axes.size()) {
                    joy.state.axes[event.number] =
                        static_cast<float>(event.value) / 32767.0f;
                    if (g_AxisCallback) {
                        g_AxisCallback(static_cast<uint32_t>(i), event.number,
                                       joy.state.axes[event.number]);
                    }
                }
            }
        }

        if (errno == ENODEV) {
            std::cout << "[Joystick] Disconnected: " << joy.state.name << "\n";
            close(joy.fd);
            joy.fd = -1;
            joy.state.connected = false;
        }
    }
}

VeraJoystickState getState(uint32_t joystickId) {
    if (joystickId < g_Joysticks.size()) return g_Joysticks[joystickId].state;
    return VeraJoystickState{};
}

void shutdown(X11Context& ctx) {
    (void)ctx;
    for (auto& joy : g_Joysticks) {
        if (joy.fd >= 0) close(joy.fd);
        joy.fd = -1;
        joy.state.connected = false;
    }
}

}  // namespace vera::x11::input
