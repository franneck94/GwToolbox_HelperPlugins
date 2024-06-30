macro(add_tb_plugin PLUGIN)
  if(CMAKE_BUILD_TYPE MATCHES Debug)
    set(PLUGIN_NAME ${PLUGIN}_debug)
  else()
    set(PLUGIN_NAME ${PLUGIN})
  endif()

  add_library(${PLUGIN_NAME} SHARED)

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
    ${PLUGIN_NAME} PRIVATE ${gwtoolboxpp_SOURCE_DIR}/plugins/Base
                           ${gwtoolboxpp_SOURCE_DIR}/GWToolboxdll)
  target_link_libraries(
    ${PLUGIN_NAME}
    PUBLIC RestClient
           directxtex
           easywsclient
           nlohmann_json::nlohmann_json
           simpleini
           imgui::fonts
           helper_utils)

  target_compile_options(${PLUGIN_NAME} PRIVATE /W4 /Gy)
  target_compile_options(${PLUGIN_NAME} PRIVATE $<$<NOT:$<CONFIG:Debug>>:/GL>)
  target_compile_options(${PLUGIN_NAME} PRIVATE $<$<CONFIG:Debug>:/ZI /Od
                                                /RTCs>)
  target_compile_options(${PLUGIN_NAME} PRIVATE $<$<CONFIG:RelWithDebInfo>:/Zi>)

  set_target_properties(
    ${PLUGIN_NAME}
    PROPERTIES VS_GLOBAL_RunCodeAnalysis false
               VS_GLOBAL_EnableMicrosoftCodeAnalysis true
               VS_GLOBAL_EnableClangTidyCodeAnalysis false)

  target_link_options(
    ${PLUGIN_NAME}
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
  target_sources(${PLUGIN_NAME} PRIVATE ${SOURCES})
  target_compile_definitions(
    ${PLUGIN_NAME} PRIVATE "NOMINMAX" "_WIN32_WINNT=_WIN32_WINNT_WIN7"
                           "WIN32_LEAN_AND_MEAN" "VC_EXTRALEAN" "BUILD_DLL")

  set_target_properties(${PLUGIN_NAME} PROPERTIES FOLDER "src")

  if(${ENABLE_WARNINGS})
    target_set_warnings(TARGET ${PLUGIN_NAME} ENABLE ${ENABLE_WARNINGS}
                        AS_ERRORS ${ENABLE_WARNINGS_AS_ERRORS})
  endif()
endmacro()
