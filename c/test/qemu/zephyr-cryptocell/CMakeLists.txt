cmake_minimum_required(VERSION 3.15)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
set(ZEPHYR_MODULES /ncs/zephyr/cmake/app/)
include(${ZEPHYR_MODULES}/boilerplate.cmake NO_POLICY_SCOPE)

include(${CMAKE_CURRENT_SOURCE_DIR}/arm-toolchain.cmake)

project(demo)

# set flags
set(CRYPTOCELL true)
set(RECORDER false)
set(CMD false)
set(TRANSPORTS false)
set(NODESELECT_DEF_WL false)
set(USE_SCRYPT false)
set(ZKSYNC false)
set(BASE64 false)
set(IPFS false)
set(MULTISIG false)
set(SOL false)
set(BTC false)
set(ETH_FULL false)
set(ED25519 false)
set(RPC_ONLY in3_signData)

set(IN3_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../../..)
set(IN3_BUILD_DIR ${CMAKE_CURRENT_BINARY_DIR}/lib)
set(IN3_LIB_DIR ${IN3_BUILD_DIR}/in3/lib)
set(CRYPTOCELL_SOURCE_DIR ${IN3_SOURCE_DIR}/c/src/signers/cryptocell)

include_directories(${CRYPTOCELL_SOURCE_DIR})
include_directories(${IN3_SOURCE_DIR}/c/include/in3)
include_directories(${IN3_SOURCE_DIR}/c/include)
include_directories(${IN3_SOURCE_DIR}/c/src/signer/cryptocell)

add_subdirectory(../../../.. in3)

set_property(TARGET cryptocell PROPERTY IMPORTED_LOCATION ${IN3_BUILD_DIR}/libcryptocell.a)
set_property(TARGET core PROPERTY IMPORTED_LOCATION ${IN3_BUILD_DIR}/libcore.a)
set_property(TARGET crypto PROPERTY IMPORTED_LOCATION ${IN3_BUILD_DIR}/libcrypto.a)
set_property(TARGET init PROPERTY IMPORTED_LOCATION ${IN3_BUILD_DIR}/libinit.a)
set_property(TARGET eth_basic PROPERTY IMPORTED_LOCATION ${IN3_BUILD_DIR}/libeth_basic.a)
set_property(TARGET eth_api PROPERTY IMPORTED_LOCATION ${IN3_BUILD_DIR}/libeth_api.a)
set_property(TARGET tommath PROPERTY IMPORTED_LOCATION ${IN3_BUILD_DIR}/libtommath.a)
set_property(TARGET pk_signer PROPERTY IMPORTED_LOCATION ${IN3_BUILD_DIR}/libpk_signer.a)

target_sources(app PRIVATE src/main.c)
target_link_libraries(app PRIVATE cryptocell core crypto init eth_basic nodeselect_def eth_nano eth_full eth_api tommath pk_signer)
