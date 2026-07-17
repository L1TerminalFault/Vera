#pragma once

#include <cstdint>
#include <string>

enum class VeraMouseButton : uint8_t {
    Left,
    Right,
    Middle,
    VeraButton4,
    VeraButton5,
    Count
};

enum class VeraCursorMode : uint8_t { Normal = 0, Hidden, Disabled };

enum class VeraCursorShape : uint8_t {
    Arrow = 0,
    IBeam,
    Crosshair,
    Hand,
    HResize,
    VResize,
    CornerResizeNWSE,
    CornerResizeNESW,
    NotAllowed,
    Count
};

struct VeraInputDeviceInfo {
    std::string name;
    bool connected;
};
