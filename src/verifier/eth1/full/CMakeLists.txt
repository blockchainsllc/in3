OPTION(EVM_GAS "if true the gas costs are verified when validating a eth_call. This is a optimization since most calls are only interessted in the result. EVM_GAS would be required if the contract uses gas-dependend op-codes." ON)
IF (EVM_GAS)
    MESSAGE(STATUS "Enable GAS in EVM")
    ADD_DEFINITIONS(-DEVM_GAS)
ENDIF (EVM_GAS)


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
target_link_libraries(eth_full eth_basic)
