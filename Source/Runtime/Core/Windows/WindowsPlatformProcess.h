#pragma once

#include <filesystem>

struct FWindowsPlatformProcess
{
    static const std::filesystem::path& ExecutablePath();
    static const std::filesystem::path& BaseDir();
};
