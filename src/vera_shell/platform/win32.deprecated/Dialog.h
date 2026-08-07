#pragma once

#include <cstdio>
#include <cstdlib>
#include <filesystem>

#include "../../core_shell/Types.h"

namespace fs = std::filesystem;

class Win32Dialog {
   public:
    static DialogResult showDialog(const DialogOptions& dopts) {
        (void)dopts;
        return DialogResult();
    }

    static fs::path openFileDialog(const FileDialogOptions& fdopts) {
        (void)fdopts;
        return "";
    }

    static fs::path saveFileDialog(const SaveFileDialogOptions& sfdopts) {
        (void)sfdopts;
        return "";
    }
};
