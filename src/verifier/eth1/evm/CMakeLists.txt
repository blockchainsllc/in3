add_library(evm_o OBJECT
        evm.c
        opcodes.c
        big.c
        call.c
        code.c
        env.c
        mem.c
        accounts.c
        gas.c
        precompiled.c)

# add gas-calculation
#ADD_DEFINITIONS(-DEVM_GAS)

# add dependency
add_library(evm STATIC $<TARGET_OBJECTS:evm_o>)
target_link_libraries(evm eth_basic)
