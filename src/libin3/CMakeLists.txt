find_package(JNI REQUIRED)
include_directories(${JNI_INCLUDE_DIRS})
include_directories ("${PROJECT_SOURCE_DIR}/src/core")
include_directories ("${PROJECT_SOURCE_DIR}/src/transport_curl")
include_directories ("${PROJECT_SOURCE_DIR}/src/eth_full")

# add the executablex
add_library( libin3 SHARED in3.c in3_IN3.c)
SET_TARGET_PROPERTIES(libin3  PROPERTIES OUTPUT_NAME in3)
# props
include(../core/compiler.cmake)

target_link_libraries (libin3 core in3_curl in3_eth_full)



