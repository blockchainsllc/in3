add_library(http_server_o OBJECT http_server.c)
target_compile_definitions(http_server_o PRIVATE -D_POSIX_C_SOURCE=199309L)

add_library(http_server STATIC $<TARGET_OBJECTS:http_server_o>)
target_link_libraries(http_server core)
if (MSVC OR MSYS OR MINGW)
    # for detecting Windows compilers
    #    target_link_libraries(transport_curl ws2_32 wsock32 pthread )
    target_link_libraries(http_server ws2_32 wsock32)
endif ()


