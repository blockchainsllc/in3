idf_component_register(SRCS in3_main.c 
                    INCLUDE_DIRS ".")


ADD_DEFINITIONS(-DDTEST)
ADD_DEFINITIONS(-DDEBUG)
ADD_DEFINITIONS(-DETH_FULL)
ADD_DEFINITIONS(-DIN3_LIB)
ADD_DEFINITIONS(-DEVM_GAS)
ADD_DEFINITIONS(-DLOGGING)
ADD_DEFINITIONS(-DIN3_EXPORT_TEST=static)
ADD_DEFINITIONS(-DIN3_IMPORT_TEST=)
ADD_DEFINITIONS(-DESP_IDF)
OPTION(ESP_IDF "Esp-idf framework" ON)
ADD_DEFINITIONS(-DIN3_MATH_LITE)
set(IN3_SRC ../../../..)
add_subdirectory(${IN3_SRC}/src/core build_core)
add_subdirectory(${IN3_SRC}/src/third-party/tommath build_tommath)
add_subdirectory(${IN3_SRC}/src/third-party/crypto build_crypto)

add_subdirectory(${IN3_SRC}/src/signer/pk-signer build_signer) 
add_subdirectory(${IN3_SRC}/src/verifier/eth1/basic build_basic)
add_subdirectory(${IN3_SRC}/src/verifier/eth1/nano build_nano)
add_subdirectory(${IN3_SRC}/src/verifier/eth1/full build_full)
add_subdirectory(${IN3_SRC}/src/verifier/eth1/evm build_evm)
add_subdirectory(${IN3_SRC}/src/api/eth1 build_eth1)
add_subdirectory(${IN3_SRC}/src/api/utils build_utils)

set(IN3_LIBS 
        $<TARGET_OBJECTS:core_o>
        $<TARGET_OBJECTS:pk_signer_o>
        $<TARGET_OBJECTS:init_o>
        $<TARGET_OBJECTS:crypto_o> 
        $<TARGET_OBJECTS:tommath_o> 
        $<TARGET_OBJECTS:eth_api_o> 
        $<TARGET_OBJECTS:eth_basic_o> 
        $<TARGET_OBJECTS:eth_nano_o>
        $<TARGET_OBJECTS:eth_full_o>
        $<TARGET_OBJECTS:evm_o>
        $<TARGET_OBJECTS:api_utils_o>
)
 
set(CMAKE_C_FLAGS "-Og -mlongcalls")
add_library(in3_bundle STATIC ${IN3_LIBS})
target_link_libraries(${COMPONENT_LIB} PUBLIC in3_bundle)
target_include_directories(${COMPONENT_LIB} PUBLIC ${IN3_SRC}/include)
