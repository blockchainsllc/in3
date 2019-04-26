find_package(JNI REQUIRED)
include_directories(${JNI_INCLUDE_DIRS})

# add the executablex
add_library( libin3 SHARED in3.c in3_IN3.c)
SET_TARGET_PROPERTIES(libin3  PROPERTIES OUTPUT_NAME in3)
# props
include(../core/compiler.cmake)

target_link_libraries (libin3 core transport_curl eth_full eth_api)



