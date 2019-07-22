OPTION(USE_CURL "if true " ON)
IF (USE_CURL)
    message("using curl for comandline")
    ADD_DEFINITIONS(-DUSE_CURL)
    set(LIBS ${LIBS} transport_curl)
else()
    message("using http for comandline")
    set(LIBS ${LIBS} transport_http)
ENDIF()

add_executable(in3 main.c in3_storage.c)

target_link_libraries(in3 ${LIBS} eth_full eth_api -lm)

install(TARGETS in3
        DESTINATION /usr/local/bin/
        PERMISSIONS
        OWNER_READ OWNER_WRITE OWNER_EXECUTE
        GROUP_READ GROUP_EXECUTE
        WORLD_READ WORLD_EXECUTE)
