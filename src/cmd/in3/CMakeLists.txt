add_executable(cin3 main.c in3_storage.c)

target_link_libraries(cin3 tommath core transport_curl eth_full eth_api -lm crypto eth_nano eth_basic)


install(TARGETS cin3
        DESTINATION /usr/local/bin/
        PERMISSIONS
        OWNER_READ OWNER_WRITE OWNER_EXECUTE
        GROUP_READ GROUP_EXECUTE
        WORLD_READ WORLD_EXECUTE)
