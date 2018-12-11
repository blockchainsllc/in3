include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup(TARGETS)

include_directories ("${PROJECT_SOURCE_DIR}/src/core")
include_directories ("${PROJECT_SOURCE_DIR}/src/transport_curl")

# add core
# add_subdirectory( ../core )



add_library( libin3 SHARED in3.c)

target_link_libraries (libin3 core in3_curl)


set_property(TARGET libin3 PROPERTY C_STANDARD 99)
