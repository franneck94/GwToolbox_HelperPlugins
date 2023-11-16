file(GLOB UTILS_SOURCES
    "*.h"
    "*.cpp")

add_library(UtilsDll STATIC ${UTILS_SOURCES})
target_compile_options(UtilsDll PUBLIC /ZI)
target_include_directories(UtilsDll PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(UtilsDll PUBLIC
    impl
    gwca
    simpleini
    fmt::fmt
    Stdafx
    TypesDll)

if (${ENABLE_WARNINGS})
    target_set_warnings(TARGET UtilsDll ENABLE ON AS_ERROR ON)
endif()
