add_library(eth_full_o OBJECT
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
add_library(eth_full STATIC $<TARGET_OBJECTS:eth_full_o>)
target_link_libraries(eth_full tommath crypto core eth_basic)
