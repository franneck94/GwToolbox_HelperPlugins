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

add_compile_definitions(
    "NOMINMAX"
    "_USE_MATH_DEFINES"
    "WIN32_LEAN_AND_MEAN"
    "SETTINGS_FOLDER=\"${CMAKE_CURRENT_LIST_DIR}\"")

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

### EXTERNAL LIBS
FetchContent_Declare(
    fmt
    GIT_REPOSITORY https://github.com/fmtlib/fmt.git
    GIT_TAG 8.1.1
    GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(fmt)

FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.2
    GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(nlohmann_json)

FetchContent_Declare(
    imgui_fonts
    GIT_REPOSITORY https://github.com/HasKha/imgui-fonts.git
    GIT_TAG 51e143ae864d141d919194af74ff916d5e6d383a)
FetchContent_MakeAvailable(imgui_fonts)

add_subdirectory(external)

include(gwtoolboxdll_plugins)

add_library(Stdafx STATIC "src/stdafx.h" "src/stdafx.cpp")
target_precompile_headers(Stdafx PUBLIC "src/stdafx.h")
target_link_libraries(Stdafx PUBLIC impl gwca)

file(GLOB UTIL_SOURCES
    "src/Utils/*.cpp"
    "src/Utils/*.cc"
    "src/Utils/*.hpp"
    "src/Utils/*.h")
add_library(helper_utils STATIC ${UTIL_SOURCES})
target_include_directories(helper_utils PUBLIC "src/Utils")
target_link_libraries(helper_utils PUBLIC gwca)
target_link_libraries(helper_utils PUBLIC impl)
target_link_libraries(helper_utils PUBLIC fmt::fmt)
target_link_libraries(helper_utils PUBLIC simpleini)
target_link_libraries(helper_utils PUBLIC Stdafx)

add_tb_plugin(AllowAllDialogs)
add_tb_plugin(LongFinger)
add_tb_plugin(RawDialogs)
add_tb_plugin(TargetEverything)
