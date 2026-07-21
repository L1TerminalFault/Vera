#pragma once

#include "../../core_shell/Types.h"

class Win32Sound {
   public:
    static bool playSound(SystemSound snd) {
        (void)snd;
        return false;
    }
};
