#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#include "core/input/Keys.h"  // Includes the VeraKey enum definition

namespace utils {

/**
 * @brief Translates Windows Virtual-Key (VK) codes and layout flags into
 * VeraKey enum values.
 */
VeraKey translateWin32KeyToVeraKey(WPARAM wparam, LPARAM lparam);

}  // namespace utils