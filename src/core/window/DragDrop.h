#pragma once
#include <cstdint>
#include <functional>
#include <string>
#include <vector>

class VeraWindow;

enum class VeraDragAction { Enter, Over, Drop, Leave };

struct VeraDragEvent {
    VeraDragAction action;
    VeraWindow* window;
    int32_t x, y;
    std::vector<std::string> paths;
};

using VeraDragCallback = std::function<bool(const VeraDragEvent&)>;
