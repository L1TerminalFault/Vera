#pragma once

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

namespace fs = std::filesystem;

class LinuxEnvironment {
   public:
    // -------------------------------------------------------------------------
    // Basic Environment Variables (Process-Level)
    // -------------------------------------------------------------------------

    static bool setEnvironmentVariable(const std::string& name,
                                       const std::string& value) {
        if (name.empty()) return false;
        // setenv(name, value, overwrite = 1) -> 0 on success
        return setenv(name.c_str(), value.c_str(), 1) == 0;
    }

    static std::optional<std::string> getEnvironmentVariable(
        const std::string& name) {
        if (name.empty()) return std::nullopt;
        const char* val = std::getenv(name.c_str());
        if (val != nullptr) {
            return std::string(val);
        }
        return std::nullopt;
    }

    static bool unsetEnvironmentVariable(const std::string& name) {
        if (name.empty()) return false;
        // unsetenv(name) -> 0 on success
        return unsetenv(name.c_str()) == 0;
    }

    // -------------------------------------------------------------------------
    // Path Operations
    // -------------------------------------------------------------------------

    static bool addPathToEnvironment(const fs::path& pathToAdd,
                                     bool persistent) {
        if (pathToAdd.empty()) return false;

        std::string targetPath = pathToAdd.lexically_normal().string();

        // 1. Process-level update
        std::optional<std::string> currentPathOpt =
            getEnvironmentVariable("PATH");
        std::string currentPath = currentPathOpt.value_or("");

        if (!containsPathEntry(currentPath, targetPath)) {
            std::string newPath = currentPath.empty()
                                      ? targetPath
                                      : (targetPath + ":" + currentPath);
            if (!setEnvironmentVariable("PATH", newPath)) {
                return false;
            }
        }

        if (!persistent) return true;

        // 2. Persistent update (Shell configs: ~/.bashrc, ~/.zshrc, ~/.profile)
        const char* homeDir = std::getenv("HOME");
        if (!homeDir) return false;

        std::vector<fs::path> targetConfigs = {fs::path(homeDir) / ".bashrc",
                                               fs::path(homeDir) / ".zshrc",
                                               fs::path(homeDir) / ".profile"};

        std::string exportStatement = "\n# Added by VeraShell\nexport PATH=\"" +
                                      targetPath + ":$PATH\"\n";

        bool persistentSuccess = false;

        for (const auto& configPath : targetConfigs) {
            // If the configuration file exists, append our path export if not
            // already there
            if (fs::exists(configPath)) {
                if (!fileContainsString(configPath, targetPath)) {
                    std::ofstream outFile(configPath, std::ios::app);
                    if (outFile.is_open()) {
                        outFile << exportStatement;
                        persistentSuccess = true;
                    }
                } else {
                    persistentSuccess = true;  // Already registered
                }
            }
        }

        return persistentSuccess;
    }

    static bool removePathFromEnvironment(const fs::path& pathToRemove,
                                          bool persistent) {
        if (pathToRemove.empty()) return false;

        std::string targetPath = pathToRemove.lexically_normal().string();

        // 1. Process-level removal
        std::optional<std::string> currentPathOpt =
            getEnvironmentVariable("PATH");
        if (currentPathOpt.has_value()) {
            std::vector<std::string> entries =
                splitPath(currentPathOpt.value());
            std::string rebuiltPath;

            for (const auto& entry : entries) {
                if (fs::path(entry).lexically_normal().string() != targetPath) {
                    if (!rebuiltPath.empty()) rebuiltPath += ":";
                    rebuiltPath += entry;
                }
            }

            setEnvironmentVariable("PATH", rebuiltPath);
        }

        if (!persistent) return true;

        // 2. Persistent removal from shell config files
        const char* homeDir = std::getenv("HOME");
        if (!homeDir) return false;

        std::vector<fs::path> targetConfigs = {fs::path(homeDir) / ".bashrc",
                                               fs::path(homeDir) / ".zshrc",
                                               fs::path(homeDir) / ".profile"};

        for (const auto& configPath : targetConfigs) {
            if (fs::exists(configPath)) {
                removeMatchingLinesFromFile(configPath, targetPath);
            }
        }

        return true;
    }

   private:
    // Helper to split a colon-separated PATH string into individual components
    static std::vector<std::string> splitPath(const std::string& pathStr) {
        std::vector<std::string> tokens;
        std::stringstream ss(pathStr);
        std::string token;
        while (std::getline(ss, token, ':')) {
            if (!token.empty()) {
                tokens.push_back(token);
            }
        }
        return tokens;
    }

    // Checks if a path entry exists inside the current process PATH variable
    static bool containsPathEntry(const std::string& pathEnv,
                                  const std::string& targetPath) {
        std::vector<std::string> entries = splitPath(pathEnv);
        for (const auto& entry : entries) {
            if (fs::path(entry).lexically_normal().string() == targetPath) {
                return true;
            }
        }
        return false;
    }

    // Helper to check if a file contains a specific substring
    static bool fileContainsString(const fs::path& filePath,
                                   const std::string& needle) {
        std::ifstream file(filePath);
        if (!file.is_open()) return false;

        std::string line;
        while (std::getline(file, line)) {
            if (line.find(needle) != std::string::npos) {
                return true;
            }
        }
        return false;
    }

    // Helper to remove line entries matching targetPath during persistent
    // cleanup
    static void removeMatchingLinesFromFile(const fs::path& filePath,
                                            const std::string& needle) {
        std::ifstream inFile(filePath);
        if (!inFile.is_open()) return;

        std::vector<std::string> lines;
        std::string line;
        bool modified = false;

        while (std::getline(inFile, line)) {
            // Strip out lines containing the specific path or the comment
            // header
            if (line.find(needle) != std::string::npos ||
                line.find("# Added by VeraShell") != std::string::npos) {
                modified = true;
                continue;
            }
            lines.push_back(line);
        }
        inFile.close();

        if (modified) {
            std::ofstream outFile(filePath, std::ios::trunc);
            if (outFile.is_open()) {
                for (const auto& l : lines) {
                    outFile << l << "\n";
                }
            }
        }
    }
};
