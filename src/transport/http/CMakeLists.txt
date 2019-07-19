add_library(transport_http_o OBJECT in3_http.c)
target_compile_definitions(transport_http_o PRIVATE -D_POSIX_C_SOURCE=199309L)

add_library(transport_http STATIC $<TARGET_OBJECTS:transport_http_o>)
target_link_libraries(transport_http core)

