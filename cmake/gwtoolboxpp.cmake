include_guard()
include(FetchContent)

# Fetch gwtooolboxpp using FetchContent
FetchContent_Declare(
  gwtoolboxpp
  GIT_REPOSITORY https://github.com/gwdevhub/gwtoolboxpp
  GIT_TAG d256890d00095e5c8d5b4a03381860ddd587065f) # between 6.20 and (upcoming) 6.21

FetchContent_GetProperties(gwtoolboxpp)

if(gwtooolboxpp_POPULATED)
  message(STATUS "Skipping gwtooolboxpp download")
  return()
endif()

FetchContent_Populate(gwtoolboxpp)

add_subdirectory(${gwtoolboxpp_SOURCE_DIR} EXCLUDE_FROM_ALL)
