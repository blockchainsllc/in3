
include_directories ("${PROJECT_SOURCE_DIR}/src/core")
include_directories ("${PROJECT_SOURCE_DIR}/src/transport_curl")


# add the executablex
add_executable(in3 main.c)
set_property(TARGET in3 PROPERTY C_STANDARD 99)
target_link_libraries (in3 core in3_curl)



