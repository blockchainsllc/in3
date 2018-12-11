include_directories ("${PROJECT_SOURCE_DIR}/src/core")

# add core
# add_subdirectory( ../core )



add_library( libin3 SHARED 
	in3.c
)
target_link_libraries (libin3 core)
set_property(TARGET libin3 PROPERTY C_STANDARD 99)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)