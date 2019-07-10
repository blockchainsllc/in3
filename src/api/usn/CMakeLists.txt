add_library(usn_api STATIC usn_api.c)
target_link_libraries(usn_api core)
target_include_directories(core PUBLIC "${PROJECT_SOURCE_DIR}/src/verifier/eth1/nano")