cmake_minimum_required(VERSION 3.16)

set(CMAKE_GENERATOR_PLATFORM win32)

project(
  "GWToolbox_HelperPlugins"
  VERSION 2.0.0
  LANGUAGES C CXX)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

if(NOT(CMAKE_SIZEOF_VOID_P EQUAL 4))
    message(FATAL_ERROR "You are configuring a non 32-bit build, this is not supported. Run cmake with `-A Win32`")
endif()

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED OFF)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/bin/")

add_compile_options(/MP /permissive-)
list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")

find_path(SIMPLEINI_INCLUDE_DIRS "ConvertUTF.c")
find_path(EARCUT_HPP_INCLUDE_DIRS "mapbox/earcut.hpp")
find_package(unofficial-uwebsockets CONFIG REQUIRED)
find_package(wolfssl CONFIG REQUIRED)
find_package(minhook CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(ctre CONFIG REQUIRED)
find_package(directxtex CONFIG REQUIRED)

add_compile_definitions(WOLFSSL_NO_OPTIONS_H)

# Third Party Libs
include(gwtoolboxpp)

# First Party Libs
file(GLOB UTIL_SOURCES "src/lib/**/*.cpp" "src/lib/**/*.cc"
     "src/lib/**/*.hpp" "src/lib/**/*.h")
add_library(helper_utils STATIC ${UTIL_SOURCES})
target_include_directories(helper_utils PUBLIC "src/lib")
target_include_directories(helper_utils PUBLIC "src/lib/Actions")
target_include_directories(helper_utils PUBLIC "src/lib/Data")
target_include_directories(helper_utils PUBLIC "src/lib/Helper")
target_include_directories(helper_utils PUBLIC "src/lib/Utils")
target_include_directories(helper_utils PUBLIC
        ${gwtoolboxpp_SOURCE_DIR}/Dependencies/GWCA/include
		${gwtoolboxpp_SOURCE_DIR}/plugins/Base
		${gwtoolboxpp_SOURCE_DIR}/GWToolboxdll
		${gwtoolboxpp_SOURCE_DIR}/Dependencies/GWCA/include
		${SIMPLEINI_INCLUDE_DIRS}
        "src/"
		RestClient
		imgui
		directxtex
		gwca
		easywsclient
		wolfssl::wolfssl
	    minhook::minhook
	    ctre::ctre
		imgui::fonts
		IconFontCppHeaders
		nativefiledialog
		wintoast
		nlohmann_json::nlohmann_json)
target_compile_options(
  helper_utils
  PRIVATE /Wall
          /wd5246
          /wd4626
          /wd4627
          /wd4626
          /wd5045
          /wd4820
          /wd5027
          /wd5259
          /wd4625
          /wd5026
          /wd5219
          /wd4623
          /wd4668
          /wd4464
          /wd4514
          /wd4201
          /wd4505
          /wd4061)

add_tb_plugin(HeroWIndow)
add_tb_plugin(SmartBip)
add_tb_plugin(SmartHexAndCondRemove)
add_tb_plugin(SmartHonor)
add_tb_plugin(SmartSkillRupts)
add_tb_plugin(SmartSoS)
add_tb_plugin(SmartSplinter)
add_tb_plugin(SmartST)
add_tb_plugin(SmartVigSpirit)
add_tb_plugin(UwDhuumBitch)
add_tb_plugin(UwEmo)
add_tb_plugin(UwMesmer)
add_tb_plugin(UwRanger)
add_tb_plugin(SmartCommands)

# Copy plugins to GWToolboxpp directory
add_custom_target(copy_plugins ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${PROJECT_SOURCE_DIR}/bin/Debug"
        "C:/Users/schaf.JANPC/Documents/GWToolboxpp/JANPC/plugins"
    DEPENDS HeroWIndow SmartBip SmartHexAndCondRemove SmartHonor SmartSkillRupts SmartSoS SmartSplinter SmartST SmartVigSpirit
    COMMENT "Copying plugin files to GWToolboxpp directory"
)
