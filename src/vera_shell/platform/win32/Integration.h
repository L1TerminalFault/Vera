#pragma once

#include <string>

#include "../../core_shell/Types.h"

class Win32Integration {
   public:
    static bool setProgress(const ProgressOptions& popts) {
        (void)popts;
        return false;
    }

    static bool clearProgress(const std::string& appId) {
        (void)appId;
        return false;
    }

    static bool requestAttention(AttentionType atype,
                                 const std::string& appId) {
        (void)atype;
        (void)appId;
        return false;
    }
};
