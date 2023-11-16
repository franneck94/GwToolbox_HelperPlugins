### SIMPLEINI
set(simpleini_SOURCES
    "simpleini/ConvertUTF.c"
    "simpleini/ConvertUTF.h"
    "simpleini/SimpleIni.h")
add_library(simpleini STATIC ${simpleini_SOURCES})
target_include_directories(simpleini PUBLIC "simpleini")
if (${ENABLE_WARNINGS})
    target_set_warnings(TARGET simpleini ENABLE OFF AS_ERROR OFF)
endif()

### GWCA
add_library(gwca)
file(GLOB GWCA_SOURCES
    "gwca/source/stdafx.h"
    "gwca/source/*.cpp"
    "gwca/include/gwca/constants/*.h"
    "gwca/include/gwca/context/*.h"
    "gwca/include/gwca/gamecontainers/*.h"
    "gwca/include/gwca/gameentities/*.h"
    "gwca/include/gwca/managers/*.h"
    "gwca/include/gwca/packets/*.h"
    "gwca/include/gwca/utilities/*.h")
target_sources(gwca PRIVATE ${GWCA_SOURCES})
target_precompile_headers(gwca PRIVATE "gwca/source/stdafx.h")
target_include_directories(gwca PUBLIC "gwca/include/")
target_link_libraries(gwca PUBLIC minhook)

### MINIHOOK
set(MINHOOK_FOLDER "${PROJECT_SOURCE_DIR}/external/GWCA/Dependencies/minhook/")
add_library(minhook)
file(GLOB MINIHOOK_SOURCES
    "${MINHOOK_FOLDER}/include/*.h"
    "${MINHOOK_FOLDER}/src/*.h"
    "${MINHOOK_FOLDER}/src/*.c"
    "${MINHOOK_FOLDER}/src/hde/*.h"
    "${MINHOOK_FOLDER}/src/hde/*.c")
source_group(TREE "${MINHOOK_FOLDER}" FILES ${MINIHOOK_SOURCES})
target_sources(minhook PRIVATE ${MINIHOOK_SOURCES})
target_include_directories(minhook PUBLIC "${MINHOOK_FOLDER}/include/")
set_target_properties(minhook PROPERTIES FOLDER "external/")

### DIRECTX
set(DIRECTX_FOLDER "${PROJECT_SOURCE_DIR}/external/DirectX")
add_library(directx INTERFACE)
target_include_directories(directx INTERFACE "${DIRECTX_FOLDER}/include")
target_link_libraries(directx INTERFACE
    "${DIRECTX_FOLDER}/lib/x86/d3d9.lib"
    "${DIRECTX_FOLDER}/lib/x86/d3dx9.lib")

### IMGUI
set(IMGUI_SOURCES
    "imgui/imgui.h"
    "imgui/imgui_internal.h"
    "imgui/imconfig.h"
    "imgui/imgui.cpp"
    "imgui/imgui_draw.cpp"
    "imgui/imgui_tables.cpp"
    "imgui/imgui_widgets.cpp")
add_library(imgui STATIC ${IMGUI_SOURCES})
target_include_directories(imgui PUBLIC "imgui")
set_target_properties(imgui PROPERTIES FOLDER "external/")

### IMPLOT
set(IMPLOT_SOURCES
    "implot/implot_internal.h"
    "implot/implot.h"
    "implot/implot_items.cpp"
    "implot/implot.cpp")
add_library(implot STATIC ${IMPLOT_SOURCES})
target_include_directories(implot PUBLIC "implot")
target_link_libraries(implot PUBLIC imgui)

### ALL GRAPHICAL LIBS
set(IMPL_SOURCES
    "imgui/backends/imgui_impl_dx9.h"
    "imgui/backends/imgui_impl_dx9.cpp"
    "imgui/backends/imgui_impl_win32.h"
    "imgui/backends/imgui_impl_win32.cpp")
add_library(impl STATIC ${IMPL_SOURCES})
target_include_directories(impl PUBLIC "imgui/backends/")
target_link_libraries(impl PUBLIC
    imgui
    implot
    directx)

if (${ENABLE_WARNINGS})
    target_set_warnings(TARGET gwca ENABLE OFF AS_ERROR OFF)
    target_set_warnings(TARGET imgui ENABLE OFF AS_ERROR OFF)
    target_set_warnings(TARGET implot ENABLE OFF AS_ERROR OFF)
    target_set_warnings(TARGET impl ENABLE OFF AS_ERROR OFF)
endif()

if(${ENABLE_LTO})
    target_enable_lto(impl optimized)
endif()
