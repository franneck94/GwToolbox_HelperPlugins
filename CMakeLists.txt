cmake_minimum_required(VERSION 3.21)

set(CMAKE_GENERATOR_PLATFORM win32)

project(
  "GWToolbox_HelperPlugins"
  VERSION 0.13.0
  LANGUAGES C CXX)

# Global CMake variables are set here
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

option(ENABLE_WARNINGS "Enable to add warnings to a target." ON)
option(ENABLE_WARNINGS_AS_ERRORS "Enable to treat warnings as errors." OFF)

# Sanity Check
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
  message(FATAL_ERROR "Run cmake with `-A Win32`")
endif()

# Settings
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set(GWTOOLBOXPP_SOURCE_DIR
    "${CMAKE_CURRENT_LIST_DIR}/build/_deps/gwtoolboxpp-src")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/bin/")

# CMAKE MODULES
set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake/)
include(FetchContent)
include(ConfigSafeGuards)
include(Warnings)

if(ENABLE_WARNINGS)
  include(Warnings)
endif()

# Third Party Libs
include(gwtoolboxpp)

# First Party Libs
add_library(Stdafx STATIC "src/stdafx.h" "src/stdafx.cpp")
target_precompile_headers(Stdafx PUBLIC "src/stdafx.h")
target_link_libraries(Stdafx PUBLIC gwca)

file(GLOB UTIL_SOURCES "src/Utils/**/*.cpp" "src/Utils/**/*.cc"
     "src/Utils/**/*.hpp" "src/Utils/**/*.h")
add_library(helper_utils STATIC ${UTIL_SOURCES})
target_include_directories(helper_utils PUBLIC "src/Utils")
target_link_libraries(helper_utils PUBLIC imgui gwca Stdafx)

include(gwtoolboxdll_plugins)

add_tb_plugin(HeroWindow)
add_tb_plugin(HeroSkills)
add_tb_plugin(UwRanger)
add_tb_plugin(UwMesmer)
add_tb_plugin(SmartCommands)
add_tb_plugin(UwDhuumBitch)
add_tb_plugin(UwEmo)
