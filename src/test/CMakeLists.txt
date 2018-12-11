include_directories ("${PROJECT_SOURCE_DIR}/src/core")


# add the executablex
add_executable(tests main.c)
set_property(TARGET tests PROPERTY C_STANDARD 99)
target_link_libraries (tests core)
