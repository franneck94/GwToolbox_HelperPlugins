#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>

#include <Logger.h>

#include "Utils.h"

std::filesystem::path GetSettingsFolderPath()
{
    const auto homepath = std::getenv("USERPROFILE");
    auto helperbox_settings_path = std::filesystem::path(homepath);
    helperbox_settings_path /= "helperbox";

    if (!std::filesystem::exists(helperbox_settings_path))
    {
        std::filesystem::create_directory(helperbox_settings_path);
    }

    return helperbox_settings_path;
}

std::filesystem::path GetPath(std::wstring_view filename)
{
    const auto filepath = GetSettingsFolderPath() / filename;

    if (!std::filesystem::exists(filepath))
    {
        std::ofstream ofs(filepath);
        ofs.close();
    }

    return filepath;
}
