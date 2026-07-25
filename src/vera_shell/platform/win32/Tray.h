#pragma once

#include "../../core_shell/Types.h"

class Win32Tray {
   public:
    static bool createTray(const TrayOptions& topts) {
        (void)topts;
        return false;
    }

    static bool updateTray(const TrayUpdate& tupdt) {
        (void)tupdt;
        return false;
    }

    static bool removeTray() { return false; }
};
