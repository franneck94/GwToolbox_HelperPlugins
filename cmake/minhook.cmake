include_guard()

set(GWCA_FOLDER "${GWTOOLBOXPP_SOURCE_DIR}/Dependencies/GWCA")
set(MINHOOK_FOLDER "${GWCA_FOLDER}/Dependencies/minhook")

file(GLOB SOURCES
    "${MINHOOK_FOLDER}/include/*.h"
    "${MINHOOK_FOLDER}/src/*.h"
    "${MINHOOK_FOLDER}/src/*.c"
    "${MINHOOK_FOLDER}/src/hde/*.h"
    "${MINHOOK_FOLDER}/src/hde/*.c")
add_library(minhook ${SOURCES})
target_include_directories(minhook PUBLIC "${MINHOOK_FOLDER}/include/")
set_target_properties(minhook PROPERTIES FOLDER "Dependencies/")
