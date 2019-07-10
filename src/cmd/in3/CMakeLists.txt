add_executable(in3 main.c in3_storage.c)

target_link_libraries(in3 tommath core transport_curl eth_full eth_api -lm)
target_include_directories(in3 PUBLIC "${PROJECT_SOURCE_DIR}/src/core")
target_include_directories(in3 PUBLIC "${PROJECT_SOURCE_DIR}/src/transport/curl")
target_include_directories(in3 PUBLIC "${PROJECT_SOURCE_DIR}/src/verifier/eth1/full")
target_include_directories(in3 PUBLIC "${PROJECT_SOURCE_DIR}/src/verifier/eth1/basic")
target_include_directories(in3 PUBLIC "${PROJECT_SOURCE_DIR}/src/api/eth1")

install(TARGETS in3
        DESTINATION /usr/local/bin/
        PERMISSIONS
        OWNER_READ OWNER_WRITE OWNER_EXECUTE
        GROUP_READ GROUP_EXECUTE
        WORLD_READ WORLD_EXECUTE)