#pragma once

#include "../../core_shell/Types.h"

class Win32Power {
   public:
    static bool preventSleep(const SleepRequest& slreq) {
        (void)slreq;
        return false;
    }

    static bool allowSleep(SleepTarget sltgt) {
        (void)sltgt;
        return false;
    }
};
