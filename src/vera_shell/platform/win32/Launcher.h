#pragma once

#include <cstdlib>

#include "../../core_shell/Types.h"

namespace fs = std::filesystem;

class Win32Launcher {
   public:
    static bool openUrl(const UrlLaunchOptions& ulopts) {
        (void)ulopts;
        return false;
    }

    static bool openFile(const FileLaunchOptions& flopts) {
        (void)flopts;
        return false;
    }

    static bool openApplication(const ApplicationLaunchOptions& alopts) {
        (void)alopts;
        return false;
    }
};
