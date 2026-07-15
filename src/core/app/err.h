#pragma once

#include <string>
enum class VeraErrorType {
    WindowCreationFailed,
    RemovedNonExistingWindow,
    DefaultError
};

struct VeraError {
    VeraErrorType type;
    std::string info;
};
