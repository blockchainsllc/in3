OPTION(EXAMPLES "examples" ON)
IF (EXAMPLES)
    add_executable(in3_example in3_example.c)
    target_link_libraries(in3_example in3_bundle -lcurl)
    target_compile_definitions(in3_example PRIVATE -D_POSIX_C_SOURCE=199309L)
    target_include_directories(in3_example PUBLIC "${PROJECT_SOURCE_DIR}/include")

    add_executable(example example.c)
    target_link_libraries(example tommath crypto core transport_curl eth_full eth_api usn_api eth_basic eth_nano)
    target_compile_definitions(example PRIVATE -D_POSIX_C_SOURCE=199309L)
    target_include_directories(example PUBLIC "${PROJECT_SOURCE_DIR}/include")

    add_executable(usn_device usn_device.c ../cmd/in3/in3_storage.c)
    target_compile_definitions(usn_device PRIVATE -D_POSIX_C_SOURCE=199309L)
    target_link_libraries(usn_device tommath crypto core transport_curl eth_full usn_api eth_basic eth_nano)
    target_include_directories(usn_device PUBLIC "${PROJECT_SOURCE_DIR}/include")

ENDIF (EXAMPLES)


