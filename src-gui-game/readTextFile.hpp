#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits.h>
#include <stdexcept>
#include <string>
#include <unistd.h>

#pragma once

// Returns the full path of the current executable.
std::string
getExecutablePath()
{
    char buffer[PATH_MAX];
    ssize_t count = readlink("/proc/self/exe", buffer, PATH_MAX);
    if (count == -1) {
        throw std::runtime_error("Failed to obtain executable path");
    }
    return std::string(buffer, static_cast<size_t>(count));
}

// Returns the directory where the executable resides.
std::string
getExecutableDir()
{
    return std::filesystem::path(getExecutablePath()).parent_path().string();
}

// Reads the contents of a file whose path is relative to the executable's
// directory.
std::string
readFromFile(const std::string& relativeFilename)
{
    // Build the absolute path by joining the executable directory and the
    // relative filename.
    std::filesystem::path fullPath =
        std::filesystem::path(getExecutableDir()) / relativeFilename;

    std::ifstream file(fullPath, std::ios::binary);
    if (!file) {
        throw std::runtime_error("Could not open " + fullPath.string());
    }
    return std::string(std::istreambuf_iterator<char>(file),
                       std::istreambuf_iterator<char>());
}
