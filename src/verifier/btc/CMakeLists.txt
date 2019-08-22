
# static lib
add_library(btc_o OBJECT
btc_merkle.c
btc_serialize.c
btc.c
        )

add_executable(btc $<TARGET_OBJECTS:btc_o>)
target_link_libraries(btc core)
