
include("${PROJECT_SOURCE_DIR}/c/compiler.cmake")

# static lib
add_library(btc_o OBJECT
        btc_merkle.c
        btc_types.c
        btc_serialize.c
        btc_target.c
        btc.c
)

add_library(btc STATIC $<TARGET_OBJECTS:btc_o>)
target_link_libraries(btc core crypto)
