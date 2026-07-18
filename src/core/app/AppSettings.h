#pragma once

#include <cstdint>

struct KeyRepeatSettings {
    uint32_t delayMs;
    uint32_t rate;
};

struct VeraSettings {
    // To be expanded with more settings
    KeyRepeatSettings keyRepeatSettings;
};
