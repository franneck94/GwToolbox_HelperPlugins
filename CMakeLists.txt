cmake_minimum_required(VERSION 3.21)

project(
    "GWToolbox_HelperPlugins"
    VERSION 1.0.0
    LANGUAGES C CXX)

# Global CMake variables are set here
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(MSVC_CRT_VERSION "VC142")

# Sanity Check
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    message(FATAL_ERROR "You are configuring a 64bit build, this is not supported. Run cmake with `-A Win32`")
endif()

# Settings
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set(GWTOOLBOXPP_SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/../GWToolboxpp")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/bin/")

# CMAKE MODULES
set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake/)
include(gwca)
include(minhook)
include(imgui)
include(imgui_fonts)
include(gwtoolboxdll_plugins)
include(simpleini)
include(json)

add_compile_definitions(
    "NOMINMAX"
    "_USE_MATH_DEFINES"
    "WIN32_LEAN_AND_MEAN"
    "SETTINGS_FOLDER=\"${CMAKE_CURRENT_LIST_DIR}\"")

add_tb_plugin(AllowAllDialogs)
add_tb_plugin(LongFinger)
add_tb_plugin(RawDialogs)
add_tb_plugin(TargetEverything)
