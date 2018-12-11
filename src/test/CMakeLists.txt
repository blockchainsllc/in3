include_directories ("${PROJECT_SOURCE_DIR}/src/core")


# add the executable
add_executable(test main.c)
set_property(TARGET test PROPERTY C_STANDARD 99)
target_link_libraries (test core)
