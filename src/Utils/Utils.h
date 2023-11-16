#pragma once

#include <ctime>
#include <filesystem>
#include <string_view>

#define VAR_NAME(v) (#v)

#define TIMER_INIT() (clock())
#define TIMER_DIFF(t) (clock() - t)

std::filesystem::path GetSettingsFolderPath();

std::filesystem::path GetPath(std::wstring_view filename);
