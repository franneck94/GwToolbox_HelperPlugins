macro(add_tb_plugin PLUGIN)
  add_library(${PLUGIN} SHARED)

  	file(GLOB SOURCES
		"src/plugins/${PLUGIN}/*.h"
		"src/plugins/${PLUGIN}/*.cpp"
		${gwtoolboxpp_SOURCE_DIR}/plugins/Base/*.h
		${gwtoolboxpp_SOURCE_DIR}/plugins/Base/*.cpp
		)

	target_include_directories(${PLUGIN} PRIVATE
		${gwtoolboxpp_SOURCE_DIR}/plugins/Base
		${gwtoolboxpp_SOURCE_DIR}/GWToolboxdll
		${gwtoolboxpp_SOURCE_DIR}/Dependencies/GWCA/include
		${SIMPLEINI_INCLUDE_DIRS}
		)
	target_link_libraries(${PLUGIN} PRIVATE
		RestClient
		imgui
		directxtex
		gwca
		easywsclient
		nlohmann_json::nlohmann_json
		wolfssl::wolfssl
	    minhook::minhook
	    ctre::ctre
		imgui::fonts
		IconFontCppHeaders
		nativefiledialog
		wintoast
        helper_utils
    )
    target_compile_options(
        ${PLUGIN} PRIVATE
        /Wall
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
        /wd4623
        /wd4668
        /wd4464
        /wd4514
        /wd4201
        /wd4505
        /wd5219
        /wd4061)

	target_link_options(${PLUGIN} PRIVATE
		"$<$<CONFIG:DEBUG>:/NODEFAULTLIB:LIBCMT>"
    )
	source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${SOURCES})
	target_sources(${PLUGIN} PRIVATE ${SOURCES})
	target_compile_definitions(${PLUGIN} PRIVATE
		"NOMINMAX"
		"_WIN32_WINNT=_WIN32_WINNT_WIN7"
		"WIN32_LEAN_AND_MEAN"
		"VC_EXTRALEAN"
		"BUILD_DLL"
    )
  target_compile_options(${PLUGIN} PRIVATE /Gy)
  target_compile_options(${PLUGIN} PRIVATE $<$<NOT:$<CONFIG:Debug>>:/GL>)
  target_compile_options(${PLUGIN} PRIVATE $<$<CONFIG:Debug>:/ZI /Od /RTCs>)
  target_compile_options(${PLUGIN} PRIVATE $<$<CONFIG:RelWithDebInfo>:/Zi>)

  set_target_properties(
    ${PLUGIN}
    PROPERTIES VS_GLOBAL_RunCodeAnalysis false
               VS_GLOBAL_EnableMicrosoftCodeAnalysis true
               VS_GLOBAL_EnableClangTidyCodeAnalysis false)
  set_target_properties(${PLUGIN} PROPERTIES FOLDER "src")

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
endmacro()
