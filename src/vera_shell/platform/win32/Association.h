#pragma once

#include <string>

#include "../../core_shell/Types.h"

namespace fs = std::filesystem;

class Win32Association {
   public:
    static bool registerAssociation(const FileAssociation& fassoc) {
        (void)fassoc;
        return false;
    }

    static bool unregisterAssociation(const std::string& mimeType) {
        (void)mimeType;
        return false;
    }

    static bool isAssociationRegistered(const std::string& mimeType) {
        (void)mimeType;
        return false;
    }
};
