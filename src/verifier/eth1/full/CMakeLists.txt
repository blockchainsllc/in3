add_library(eth_full_o OBJECT
        eth_full.c
        )

# add gas-calculation
#ADD_DEFINITIONS(-DEVM_GAS)

# add dependency
add_library(eth_full STATIC $<TARGET_OBJECTS:eth_full_o>)
target_link_libraries(eth_full evm)
