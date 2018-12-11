include(FindCURL)
find_package(CURL REQUIRED)


include_directories ("${PROJECT_SOURCE_DIR}/src/core")
include_directories ("${PROJECT_SOURCE_DIR}/src/transport_curl")


# add the executablex
add_executable(tests main.c)
set_property(TARGET tests PROPERTY C_STANDARD 99)
target_link_libraries (tests core in3_curl curl)



