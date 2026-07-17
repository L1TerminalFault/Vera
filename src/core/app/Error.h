#pragma once

#include <string>

namespace vera::core::app {

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

}  // namespace vera::core::app
