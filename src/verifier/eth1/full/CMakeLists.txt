add_library(eth_full STATIC
        eth_full.c
        evm.c
        opcodes.c
        big.c
        call.c
        code.c
        env.c
        mem.c
        precompiled.c)

# add gas-calculation
#ADD_DEFINITIONS(-DEVM_GAS)

# add dependency
target_link_libraries(eth_full tommath crypto core eth_basic)
target_include_directories(eth_full PUBLIC "${PROJECT_SOURCE_DIR}/src/core")
target_include_directories(eth_full PUBLIC "${PROJECT_SOURCE_DIR}/src/third-party/tommath")
target_include_directories(eth_full PUBLIC "${PROJECT_SOURCE_DIR}/src/verifier/eth1/nano")