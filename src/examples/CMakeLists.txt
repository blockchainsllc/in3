OPTION(EXAMPLES "examples" ON)
IF (EXAMPLES)

    add_executable(example example.c)
    target_link_libraries(example tommath crypto core transport_curl eth_full eth_api usn_api)
    target_compile_definitions(example PRIVATE -D_POSIX_C_SOURCE=199309L)
    target_include_directories(example PUBLIC "${PROJECT_SOURCE_DIR}/include")

    add_executable(usn_device usn_device.c ../cmd/in3/in3_storage.c)
    target_compile_definitions(usn_device PRIVATE -D_POSIX_C_SOURCE=199309L)
    target_link_libraries(usn_device tommath crypto core transport_curl eth_full usn_api)
    target_include_directories(usn_device PUBLIC "${PROJECT_SOURCE_DIR}/include")

ENDIF (EXAMPLES)


