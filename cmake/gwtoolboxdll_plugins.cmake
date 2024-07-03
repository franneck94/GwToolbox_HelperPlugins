macro(add_tb_plugin PLUGIN)
  add_library(${PLUGIN} SHARED)

  file(
    GLOB
    SOURCES
    "src/${PLUGIN}/*.h"
    "src/${PLUGIN}/*.cpp"
    "src/${PLUGIN}/*.ixx"
    ${gwtoolboxpp_SOURCE_DIR}/plugins/Base/*.h
    ${gwtoolboxpp_SOURCE_DIR}/plugins/Base/*.ixx
    ${gwtoolboxpp_SOURCE_DIR}/plugins/Base/*.cpp)

  target_include_directories(
    ${PLUGIN} PRIVATE ${gwtoolboxpp_SOURCE_DIR}/plugins/Base
                      ${gwtoolboxpp_SOURCE_DIR}/GWToolboxdll
                      src/${PLUGIN})
  target_link_libraries(
    ${PLUGIN}
    PUBLIC RestClient
           directxtex
           easywsclient
           nlohmann_json::nlohmann_json
           simpleini
           imgui::fonts
           helper_utils)

  target_compile_options(${PLUGIN} PRIVATE /Wall /Wx /Gy)
  target_compile_options(${PLUGIN} PRIVATE $<$<NOT:$<CONFIG:Debug>>:/GL>)
  target_compile_options(${PLUGIN} PRIVATE $<$<CONFIG:Debug>:/ZI /Od /RTCs>)
  target_compile_options(${PLUGIN} PRIVATE $<$<CONFIG:RelWithDebInfo>:/Zi>)

  set_target_properties(
    ${PLUGIN}
    PROPERTIES VS_GLOBAL_RunCodeAnalysis false
               VS_GLOBAL_EnableMicrosoftCodeAnalysis true
               VS_GLOBAL_EnableClangTidyCodeAnalysis false)

  target_link_options(
    ${PLUGIN}
    PRIVATE
    /WX
    /OPT:REF
    /OPT:ICF
    /SAFESEH:NO
    $<$<NOT:$<CONFIG:Debug>>:/INCREMENTAL:NO>
    $<$<CONFIG:Debug>:/IGNORE:4098
    /OPT:NOREF
    /OPT:NOICF>
    $<$<CONFIG:RelWithDebInfo>:/OPT:NOICF>
    $<$<CONFIG:DEBUG>:/NODEFAULTLIB:LIBCMT>
    /IGNORE:4099 # pdb not found for github action
  )

  source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${SOURCES})
  target_sources(${PLUGIN} PRIVATE ${SOURCES})
  target_compile_definitions(
    ${PLUGIN} PRIVATE "NOMINMAX" "_WIN32_WINNT=_WIN32_WINNT_WIN7"
                      "WIN32_LEAN_AND_MEAN" "VC_EXTRALEAN" "BUILD_DLL")

  set_target_properties(${PLUGIN} PROPERTIES FOLDER "src")
endmacro()
