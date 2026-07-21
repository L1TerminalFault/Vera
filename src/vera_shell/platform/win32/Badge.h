#pragma once

#include <string>

#include "../../core_shell/Types.h"

class Win32Badge {
   public:
    static bool setBadge(const BadgeOptions& bopts) {
        (void)bopts;
        return false;
    }

    static bool clearBadge(const std::string& appId) {
        (void)appId;
        return false;
    }
};
