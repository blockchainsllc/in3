add_executable(rlp rlp.c)
target_link_libraries(rlp crypto core eth_nano)

add_executable(json data.c)
target_link_libraries(json crypto core)
