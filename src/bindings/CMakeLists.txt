find_package(JNI REQUIRED)
include_directories(${JNI_INCLUDE_DIRS})

# add the executablex
add_library( libin3 SHARED in3.c in3_IN3.c)
SET_TARGET_PROPERTIES(libin3  PROPERTIES OUTPUT_NAME in3)
# props
include("${PROJECT_SOURCE_DIR}/compiler.cmake")

target_link_libraries (libin3 core transport_curl eth_full)
target_include_directories(libin3 PUBLIC "${PROJECT_SOURCE_DIR}/src/transport/curl")
target_include_directories(eth_full PUBLIC "${PROJECT_SOURCE_DIR}/src/verifier/eth1/full")



