add_library(eth_api STATIC eth_api.c abi.c key.c)
target_link_libraries(eth_api eth_nano core crypto)
target_include_directories(core PUBLIC "${PROJECT_SOURCE_DIR}/src/third-party")
