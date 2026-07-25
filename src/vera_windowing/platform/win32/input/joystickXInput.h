#include <xinput.h>
#include "platform\win32\window\vera_win32.h"
#pragma comment(lib, "xinput.lib")

namespace utils {
inline void pollWin32Joysticks(VeraWin32Window* activeWindow) {
    if (!activeWindow) return;

    for (DWORD joyId = 0; joyId < 4; ++joyId) {
        XINPUT_STATE state{};
        if (XInputGetState(joyId, &state) != ERROR_SUCCESS) {
            continue; 
        }

        if (activeWindow->m_joystick_button_callback) {
            static WORD lastButtons[4] = {0};
            WORD currentButtons = state.Gamepad.wButtons;

            if (currentButtons != lastButtons[joyId]) {
                WORD buttonMasks[] = {XINPUT_GAMEPAD_A,
                                      XINPUT_GAMEPAD_B,
                                      XINPUT_GAMEPAD_X,
                                      XINPUT_GAMEPAD_Y,
                                      XINPUT_GAMEPAD_LEFT_SHOULDER,
                                      XINPUT_GAMEPAD_RIGHT_SHOULDER,
                                      XINPUT_GAMEPAD_DPAD_UP,
                                      XINPUT_GAMEPAD_DPAD_DOWN,
                                      XINPUT_GAMEPAD_DPAD_LEFT,
                                      XINPUT_GAMEPAD_DPAD_RIGHT};

                for (uint32_t btnIndex = 0; btnIndex < std::size(buttonMasks);
                     ++btnIndex) {
                    bool wasPressed =
                        (lastButtons[joyId] & buttonMasks[btnIndex]) != 0;
                    bool isPressed =
                        (currentButtons & buttonMasks[btnIndex]) != 0;

                    if (isPressed != wasPressed) {
                        activeWindow->m_joystick_button_callback(
                            joyId, btnIndex, isPressed);
                    }
                }
                lastButtons[joyId] = currentButtons;
            }
        }

        if (activeWindow->m_joystick_axis_callback) {
            float leftStickX =
                static_cast<float>(state.Gamepad.sThumbLX) / 32767.0f;
            float leftStickY =
                static_cast<float>(state.Gamepad.sThumbLY) / 32767.0f;

            activeWindow->m_joystick_axis_callback(joyId, 0, leftStickX);
            activeWindow->m_joystick_axis_callback(joyId, 1, leftStickY);
        }
    }
}
}