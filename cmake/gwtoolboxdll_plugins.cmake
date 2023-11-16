add_library(plugin_base INTERFACE)
target_sources(plugin_base INTERFACE
    "src/Base/dllmain.cpp"
    "src/Base/stl.h"
    "src/Base/ToolboxPlugin.h"
    "src/Base/ToolboxPlugin.cpp"
    "src/Base/PluginUtils.ixx"
    "src/Base/ToolboxUIPlugin.h"
    "src/Base/ToolboxUIPlugin.cpp")
target_include_directories(plugin_base INTERFACE
    "src/Base"
    "${GWTOOLBOXPP_SOURCE_DIR}/GWToolboxdll")
target_link_libraries(plugin_base INTERFACE
    imgui
    gwca
    simpleini
    IconFontCppHeaders
    nlohmann_json::nlohmann_json
    imgui::fonts)
target_compile_definitions(plugin_base INTERFACE BUILD_DLL)

macro(add_tb_plugin PLUGIN)
    add_library(${PLUGIN} SHARED)
    file(GLOB SOURCES
        "${PROJECT_SOURCE_DIR}/src/${PLUGIN}/*.h"
        "${PROJECT_SOURCE_DIR}/src/${PLUGIN}/*.cpp")
    target_sources(${PLUGIN} PRIVATE ${SOURCES})
    target_include_directories(${PLUGIN} PRIVATE "${PROJECT_SOURCE_DIR}/src/${PLUGIN}/")
    target_link_libraries(${PLUGIN} PRIVATE plugin_base)
    target_compile_options(${PLUGIN} PRIVATE /wd4201 /wd4505)
    target_compile_options(${PLUGIN} PRIVATE /W4 /WX /Gy)
    target_compile_options(${PLUGIN} PRIVATE $<$<NOT:$<CONFIG:Debug>>:/GL>)
    target_compile_options(${PLUGIN} PRIVATE $<$<CONFIG:Debug>:/ZI /Od>)
    target_link_options(${PLUGIN} PRIVATE /WX /OPT:REF /OPT:ICF /SAFESEH:NO)
    target_link_options(${PLUGIN} PRIVATE $<$<NOT:$<CONFIG:Debug>>:/LTCG /INCREMENTAL:NO>)
    target_link_options(${PLUGIN} PRIVATE $<$<CONFIG:Debug>:/IGNORE:4098 /OPT:NOREF /OPT:NOICF>)
    target_link_options(${PLUGIN} PRIVATE $<$<CONFIG:RelWithDebInfo>:/OPT:NOICF>)
    set_target_properties(${PLUGIN} PROPERTIES FOLDER "${PROJECT_SOURCE_DIR}/src/${PLUGIN}")
endmacro()
