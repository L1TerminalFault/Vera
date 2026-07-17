#pragma once

#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iostream>

#include "platform/wayland/internal/WaylandInternal.hxx"

namespace vera::wayland::desktop::theme {

using namespace internal;

enum class VeraThemeMode : uint8_t { Light = 0, Dark };

static VeraThemeMode g_CurrentThemeMode = VeraThemeMode::Dark;

static VeraThemeMode detectDesktopPreference() {
    const char* gtkThemeEnv = std::getenv("GTK_THEME");
    if (gtkThemeEnv) {
        std::string theme(gtkThemeEnv);
        if (theme.find("dark") != std::string::npos ||
            theme.find("Dark") != std::string::npos) {
            return VeraThemeMode::Dark;
        }
    }

    const char* homeDir = std::getenv("HOME");
    if (homeDir) {
        std::string configPath =
            std::string(homeDir) + "/.config/gtk-3.0/settings.ini";
        std::ifstream file(configPath);
        if (file.is_open()) {
            std::string line;
            while (std::getline(file, line)) {
                if (line.find("gtk-application-prefer-dark-theme") !=
                    std::string::npos) {
                    if (line.find("true") != std::string::npos ||
                        line.find("1") != std::string::npos) {
                        return VeraThemeMode::Dark;
                    } else {
                        return VeraThemeMode::Light;
                    }
                }
            }
        }
    }

    return VeraThemeMode::Dark;
}

void initialize(WaylandContext& ctx) {
    (void)ctx;
    g_CurrentThemeMode = detectDesktopPreference();
    std::cout << "[Theme] Initialized color mode preference: "
              << (g_CurrentThemeMode == VeraThemeMode::Dark ? "Dark" : "Light")
              << "\n";
}

VeraThemeMode getActiveMode() { return g_CurrentThemeMode; }

void update() {
    static uint32_t frameCount = 0;
    frameCount++;

    if (frameCount % 300 == 0) {
        VeraThemeMode detected = detectDesktopPreference();
        if (detected != g_CurrentThemeMode) {
            g_CurrentThemeMode = detected;
            std::cout << "[Theme] Desktop theme changed dynamically to: "
                      << (g_CurrentThemeMode == VeraThemeMode::Dark ? "Dark"
                                                                    : "Light")
                      << "\n";
        }
    }
}

void shutdown() {}

}  // namespace vera::wayland::desktop::theme
