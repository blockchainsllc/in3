add_executable(rlp rlp.c)
target_link_libraries(rlp eth_nano)

add_executable(json data.c)
target_link_libraries(json core)
install(TARGETS rlp
        DESTINATION /usr/local/bin/
        PERMISSIONS
        OWNER_READ OWNER_WRITE OWNER_EXECUTE
        GROUP_READ GROUP_EXECUTE
        WORLD_READ WORLD_EXECUTE)

