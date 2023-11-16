file(GLOB HELPER_SOURCES
    "*.h"
    "*.cpp")

add_library(HelperDll STATIC ${HELPER_SOURCES})
target_compile_options(HelperDll PUBLIC /ZI)
target_include_directories(HelperDll PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(HelperDll PUBLIC
    impl
    gwca
    simpleini
    fmt::fmt
    Stdafx
    UtilsDll
    TypesDll
    DataDll)

if (${ENABLE_WARNINGS})
    target_set_warnings(TARGET HelperDll ENABLE ON AS_ERROR ON)
endif()
