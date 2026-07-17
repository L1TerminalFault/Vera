#pragma once

#include <string>

enum class VeraErrorType {
    WindowCreationFailed,
    RemovedNonExistingWindow,
    BackendInitFailed,
    UnsupportedOperation,
    DefaultError
};

struct VeraError {
    VeraErrorType type;
    std::string info;
};
