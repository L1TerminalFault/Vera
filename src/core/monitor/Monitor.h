#pragma once

#include <cstdint>
#include <optional>
#include <string>

struct VeraMonitorInfo {
    std::string name;
    int32_t x, y;
    int32_t workAreaX, workAreaY;
    uint32_t workAreaWidth, workAreaHeight;
    float dpiScale;
    uint32_t refreshRateHz;
    bool isPrimary;
    std::optional<uint32_t> physicalWidthMm, physicalHeightMm;
};

struct VeraDisplayModeInfo {
    uint32_t width, height;
    uint32_t refreshRateHz;
    uint32_t bitsPerPixel;
};
