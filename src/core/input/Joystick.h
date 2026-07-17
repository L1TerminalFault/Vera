#pragma once

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

struct VeraJoystickState {
    std::string name;
    bool connected = false;
    std::vector<float> axes;
    std::vector<bool> buttons;
};

using VeraJoystickButtonCallback =
    std::function<void(uint32_t joyId, uint32_t btn, bool pressed)>;
using VeraJoystickAxisCallback =
    std::function<void(uint32_t joyId, uint32_t axis, float val)>;
