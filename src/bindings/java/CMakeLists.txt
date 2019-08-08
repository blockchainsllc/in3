
add_library(libin3 SHARED in3.c in3_IN3.c)
SET_TARGET_PROPERTIES(libin3 PROPERTIES OUTPUT_NAME in3)

target_link_libraries(libin3  eth_full)

find_package(Java REQUIRED)
find_package(JNI REQUIRED)
include(UseJava)

project(in3j)

include_directories(${JNI_INCLUDE_DIRS})
#set(CMAKE_JAVA_COMPILE_FLAGS "-source" "1.6" "-target" "1.6")

add_jar(in3j 
  in3/IN3.java 
  in3/JSON.java 
  in3/StorageProvider.java 
  in3/Proof.java 

  in3/eth1/EthAPI.java
  in3/eth1/Block.java
  
  OUTPUT_DIR ${CMAKE_BINARY_DIR}/lib 
  OUTPUT_NAME in3
)


message(STATUS "Jar file ${_jarFile}")
message(STATUS "Class compiled to ${_classDir}")