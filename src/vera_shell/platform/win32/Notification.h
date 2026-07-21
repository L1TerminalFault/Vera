#pragma once

#include <cstdint>

#include "../../core_shell/Types.h"

class Win32Notification {
   public:
    static NotificationResult showNotification(
        const NotificationOptions& nopts) {
        (void)nopts;
        return NotificationResult();
    }

    static bool closeNotification(std::uint32_t id) {
        (void)id;
        return false;
    }
};
