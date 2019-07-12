add_executable(in3 main.c in3_storage.c)

target_link_libraries(in3 tommath core transport_curl eth_full eth_api -lm)


install(TARGETS in3
        DESTINATION /usr/local/bin/
        PERMISSIONS
        OWNER_READ OWNER_WRITE OWNER_EXECUTE
        GROUP_READ GROUP_EXECUTE
        WORLD_READ WORLD_EXECUTE)