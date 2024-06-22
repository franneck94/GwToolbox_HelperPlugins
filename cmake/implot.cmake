file(GLOB IMPLOT_SOURCES "${CMAKE_SOURCE_DIR}/external/implot/*.cpp"
     "${CMAKE_SOURCE_DIR}/external/implot/*.cpp")
add_library(implot STATIC ${IMPLOT_SOURCES})
target_include_directories(implot PUBLIC "${CMAKE_SOURCE_DIR}/external/implot")
target_link_libraries(implot PUBLIC imgui)
