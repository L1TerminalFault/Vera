#pragma once

#include <cstdint>

enum class VeraLinuxProtocol : uint8_t {
    Auto = 0,
    Wayland,
    X11,
};

enum class VeraSystemTheme { Light, Dark, Unknown };

struct VeraAppInfo {
    bool enablePlatformDebugging = false;
    VeraLinuxProtocol preferedLinuxProtocol = VeraLinuxProtocol::Auto;
};
