#pragma once

#include <cstdint>

struct VeraNativeHandle {
    void* hwnd = nullptr;
    void* display = nullptr;
    uint64_t x11Window = 0;
    void* waylandSurface = nullptr;
};
