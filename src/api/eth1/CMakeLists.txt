add_library(eth_api OBJECT eth_api.c abi.c key.c)
target_compile_definitions(eth_api PRIVATE -D_POSIX_C_SOURCE=199309L)
target_link_libraries(eth_api eth_nano core crypto)
