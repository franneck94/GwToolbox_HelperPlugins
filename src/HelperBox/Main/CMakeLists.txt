file(GLOB MAIN_SOURCES
    "Base/*.h"
    "Base/*.cpp"
    "Features/*.h"
    "Features/*.cpp"
    "Features/Uw/*.h"
    "Features/Uw/*.cpp"
    "Features/General/*.h"
    "Features/General/*.cpp"
    "*.h"
    "*.cpp")

add_library(MainDll STATIC ${MAIN_SOURCES})
target_compile_options(MainDll PUBLIC /ZI)
target_include_directories(MainDll PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(MainDll PUBLIC
    impl
    gwca
    simpleini
    fmt::fmt
    Stdafx
    UtilsDll
    TypesDll
    DataDll
    HelperDll)

if (${ENABLE_WARNINGS})
    target_set_warnings(TARGET MainDll ENABLE ON AS_ERROR OFF)
endif()
