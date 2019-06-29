include(${PROJECT_SOURCE_DIR}/src/core/compiler.cmake)

get_property(DEAD_STRIP_LINKER_OPT GLOBAL PROPERTY DEAD_STRIP_LINKER_OPT)

# add the executablex
add_executable( in3        main.c)
add_executable( example    example.c)
add_executable( json       data.c)
add_executable( rlp        rlp.c)
add_executable( usn_device usn_device.c )

set_property(TARGET example PROPERTY C_STANDARD 99)
set_property(TARGET in3 PROPERTY C_STANDARD 99)
set_property(TARGET json PROPERTY C_STANDARD 99)
set_property(TARGET rlp PROPERTY C_STANDARD 99)
set_property(TARGET usn_device PROPERTY C_STANDARD 99)

target_link_libraries (usn_device usn_api core transport_curl eth_full ${DEAD_STRIP_LINKER_OPT})
target_link_libraries (in3 core transport_curl eth_full eth_api -lm ${DEAD_STRIP_LINKER_OPT})
target_link_libraries (example core transport_curl eth_full eth_api usn_api ${DEAD_STRIP_LINKER_OPT})
target_link_libraries (rlp core eth_nano ${DEAD_STRIP_LINKER_OPT})
target_link_libraries (json core ${DEAD_STRIP_LINKER_OPT})

