file(GLOB DATA_SOURCES
    "*.h"
    "*.cpp")

add_library(DataDll STATIC ${DATA_SOURCES})
target_compile_options(DataDll PUBLIC /ZI)
target_include_directories(DataDll PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(DataDll PUBLIC
    impl
    gwca
    simpleini
    fmt::fmt
    Stdafx
    UtilsDll
    TypesDll)

if (${ENABLE_WARNINGS})
    target_set_warnings(TARGET DataDll ENABLE ON AS_ERROR ON)
endif()
