#pragma once

#include <canberra.h>  // libcanberra standard XDG sound library

#include "../../core_shell/Types.h"

class LinuxSound {
   public:
    static bool playSound(SystemSound snd) {
        const char* soundId = translateSoundToId(snd);
        if (!soundId) {
            return false;
        }

        ca_context* ctx = nullptr;
        int res = ca_context_create(&ctx);
        if (res != CA_SUCCESS || !ctx) {
            return false;
        }

        // Play sound event asynchronously using active Freedesktop sound theme
        res = ca_context_play(ctx,
                              0,  // Internal sound slot ID
                              CA_PROP_EVENT_ID,
                              soundId,  // Freedesktop sound event ID string
                              nullptr);

        ca_context_destroy(ctx);
        return (res == CA_SUCCESS);
    }

   private:
    static const char* translateSoundToId(SystemSound snd) {
        // Maps SystemSound enum values to official XDG Sound Theme
        // Specification identifiers
        switch (snd) {
            case SystemSound::Notification:
                return "message-new-instant";  // Standard notification chime
            case SystemSound::Warning:
                return "dialog-warning";  // System warning sound
            case SystemSound::Error:
                return "dialog-error";  // System error sound
            case SystemSound::Question:
                return "dialog-question";  // Prompt / question sound
            case SystemSound::Success:
                return "complete";  // Task completion / success sound
            default:
                return nullptr;
        }
    }
};
