#pragma once

#include <string>
enum class VeraErrorType {
    WindowCreationFailed,
    RemovedNonExistingWindow,
    CreationFailed,
    DefaultError
};

struct VeraError {
    VeraErrorType type;
    std::string info;
};
