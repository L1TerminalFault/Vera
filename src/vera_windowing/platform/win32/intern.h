#if defined(_WIN32)

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

void pollPlatformEvents() {}

void waitPlatformEvents() {
    MSG msg;
    if (GetMessageW(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
}

void waitPlatformEventsTimeout(double timeout_seconds) {
    DWORD timeout_ms = static_cast<DWORD>(timeout_seconds * 1000.0);
    MsgWaitForMultipleObjectsEx(0, nullptr, timeout_ms, QS_ALLINPUT,
                                MWMO_INPUTAVAILABLE);
    pollPlatformEvents();
}

#endif
