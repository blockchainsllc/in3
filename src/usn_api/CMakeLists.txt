
# static lib
add_library( usn_api STATIC usn_api.c )


# include core in the headers
include_directories ("${PROJECT_SOURCE_DIR}/src/core")

# add dependency
target_link_libraries(usn_api core eth_api)

# props
include(../core/compiler.cmake)
target_include_directories(usn_api PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
