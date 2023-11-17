cmake_minimum_required(VERSION 3.21)

set(CMAKE_GENERATOR_PLATFORM win32)

project(
    "GWToolbox_HelperPlugins"
    VERSION 1.0.0
    LANGUAGES C CXX)

# Global CMake variables are set here
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

# Sanity Check
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    message(FATAL_ERROR "You are configuring a 64bit build, this is not supported. Run cmake with `-A Win32`")
endif()

# Settings
add_definitions(-DGWCA_CTOS_ENABLED=1)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set(GWTOOLBOXPP_SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/../GWToolboxpp")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/bin/")

# CMAKE MODULES
set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake/)
include(FetchContent)
include(ConfigSafeGuards)
include(Warnings)

# Third Party Libs
include(gwtoolboxpp)

FetchContent_Declare(
    fmt
    GIT_REPOSITORY https://github.com/fmtlib/fmt.git
    GIT_TAG 8.1.1
    GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(fmt)

FetchContent_Declare(
    implot
    GIT_REPOSITORY https://github.com/epezent/implot
    GIT_TAG v0.14)
FetchContent_MakeAvailable(implot)

# First Party Libs
add_library(Stdafx STATIC "src/stdafx.h" "src/stdafx.cpp")
target_precompile_headers(Stdafx PUBLIC "src/stdafx.h")
target_link_libraries(Stdafx PUBLIC gwca)

file(GLOB UTIL_SOURCES
    "src/Utils/*.cpp"
    "src/Utils/*.cc"
    "src/Utils/*.hpp"
    "src/Utils/*.h")
add_library(helper_utils STATIC ${UTIL_SOURCES})
target_include_directories(helper_utils PUBLIC "src/Utils")
target_link_libraries(helper_utils PUBLIC
    RestClient
    imgui
    directxtex
    gwca
    earcut
    easywsclient
    discord_game_sdk
    nlohmann_json::nlohmann_json
    simpleini
    imgui::fonts
    IconFontCppHeaders
    nativefiledialog
    wintoast
    Stdafx
    fmt::fmt)

include(gwtoolboxdll_plugins)

add_tb_plugin(AllowAllDialogs)  # Does not work currently
add_tb_plugin(LongFinger)
add_tb_plugin(RawDialogs)
add_tb_plugin(TargetEverything)

add_tb_plugin(AutoFollow)  # To be Tweaked
add_tb_plugin(HeroWindow)
add_tb_plugin(SmartCommands)  # Dhuum Command needs be checked
add_tb_plugin(UwRanger)
add_tb_plugin(UwMesmer)  # Tobe Tweaked
add_tb_plugin(UwDhuumStats)  # To be checked
add_tb_plugin(UwDhuumBitch)  # To be implemented correct
add_tb_plugin(UwEmo)  # To be implemented correct
