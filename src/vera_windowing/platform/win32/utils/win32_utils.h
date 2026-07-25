#pragma once

#ifndef WIN32_LEAN_AND_MEAN
    #define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <string>

namespace utils {

/**
 * @brief Converts a UTF-8 string to a UTF-16 wide string .
 */
inline std::wstring utf8_to_wide(const std::string& utf8) {
    if (utf8.empty()) return L"";
    int sizeNeeded = MultiByteToWideChar(
        CP_UTF8, 0, utf8.c_str(), static_cast<int>(utf8.size()), nullptr, 0);
    std::wstring wideStr(sizeNeeded, 0);
    MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), static_cast<int>(utf8.size()),
                        &wideStr[0], sizeNeeded);
    return wideStr;
}

/**
 * @brief Converts a UTF-16 wide string to a UTF-8 string.
 */
inline std::string wide_to_utf8(const std::wstring& wide) {
    if (wide.empty()) return "";
    int sizeNeeded = WideCharToMultiByte(CP_UTF8, 0, wide.c_str(),
                                         static_cast<int>(wide.size()), nullptr,
                                         0, nullptr, nullptr);
    std::string utf8Str(sizeNeeded, 0);
    WideCharToMultiByte(CP_UTF8, 0, wide.c_str(), static_cast<int>(wide.size()),
                        &utf8Str[0], sizeNeeded, nullptr, nullptr);
    return utf8Str;
}

}
