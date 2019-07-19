add_library(eth_api_o OBJECT eth_api.c abi.c key.c)
target_compile_definitions(eth_api_o PRIVATE -D_POSIX_C_SOURCE=199309L)

add_library(eth_api STATIC $<TARGET_OBJECTS:eth_api_o>)
target_link_libraries(eth_api eth_nano)
