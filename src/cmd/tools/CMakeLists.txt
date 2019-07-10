add_executable(rlp rlp.c)
target_link_libraries(rlp crypto core eth_nano)
target_include_directories(rlp PUBLIC "${PROJECT_SOURCE_DIR}/src/core")
target_include_directories(rlp PUBLIC "${PROJECT_SOURCE_DIR}/src/verifier/eth1/nano")

add_executable(json data.c)
target_link_libraries(json crypto core)
target_include_directories(json PUBLIC "${PROJECT_SOURCE_DIR}/src/core")
target_include_directories(json PUBLIC "${PROJECT_SOURCE_DIR}/src/verifier/eth1/nano")
