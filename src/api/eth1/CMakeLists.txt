find_package(scrypt)

if (${scrypt_FOUND})
MESSAGE(STATUS "Found Scrypt in ${scrypt_INCLUDE_DIRS})")
include_directories(${scrypt_INCLUDE_DIRS})
  set(LIBS ${LIBS} ${scrypt_LIBRARIES})
  ADD_DEFINITIONS(-DSCRYPT)
endif()

add_library(eth_api STATIC eth_api.c abi.c key.c)
target_link_libraries(eth_api eth_nano core crypto ${LIBS})

