OPTION(EXAMPLES "examples" OFF)
IF (EXAMPLES)

    add_executable(example example.c)
    target_link_libraries(example tommath crypto core transport_curl eth_full eth_api usn_api)
    target_include_directories(example PUBLIC "${PROJECT_SOURCE_DIR}/src/core")
    target_include_directories(example PUBLIC "${PROJECT_SOURCE_DIR}/src/transport/curl")
    target_include_directories(example PUBLIC "${PROJECT_SOURCE_DIR}/src/verifier/eth1/full")
    target_include_directories(example PUBLIC "${PROJECT_SOURCE_DIR}/src/verifier/eth1/basic")
    target_include_directories(example PUBLIC "${PROJECT_SOURCE_DIR}/src/api/eth1")
    target_include_directories(example PUBLIC "${PROJECT_SOURCE_DIR}/src/api/usn")

    add_executable(usn_device usn_device.c ../cmd/in3/in3_storage.c)
    target_link_libraries(usn_device tommath crypto core transport_curl eth_full usn_api)
    target_include_directories(usn_device PUBLIC "${PROJECT_SOURCE_DIR}/src/verifier/eth1/full")
    target_include_directories(usn_device PUBLIC "${PROJECT_SOURCE_DIR}/src/transport/curl")
    target_include_directories(usn_device PUBLIC "${PROJECT_SOURCE_DIR}/src/api/usn")


ENDIF (EXAMPLES)


