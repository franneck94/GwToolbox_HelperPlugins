file(GLOB TYPES_SOURCES
    "*.h"
    "*.cpp")

add_library(TypesDll STATIC ${TYPES_SOURCES})
target_compile_options(TypesDll PUBLIC /ZI)
target_include_directories(TypesDll PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(TypesDll PUBLIC
    impl
    gwca
    simpleini
    fmt::fmt
    Stdafx
    UtilsDll
    DataDll
    HelperDll)

if (${ENABLE_WARNINGS})
    target_set_warnings(TARGET TypesDll ENABLE ON AS_ERROR ON)
endif()
