
add_library(libin3 SHARED in3.c in3_IN3.c)
SET_TARGET_PROPERTIES(libin3 PROPERTIES OUTPUT_NAME in3)

target_link_libraries(libin3 transport_curl eth_full)

find_package(Java REQUIRED)
find_package(JNI REQUIRED)
include(UseJava)

project(in3j)

include_directories(${JNI_INCLUDE_DIRS})
#set(CMAKE_JAVA_COMPILE_FLAGS "-source" "1.6" "-target" "1.6")

add_jar(in3j in3/IN3.java in3/StorageProvider.java in3/Proof.java)

#get_target_property(_jarFile in3j JAR_FILE)
#get_target_property(_classDir in3j CLASSDIR)

message(STATUS "Jar file ${_jarFile}")
message(STATUS "Class compiled to ${_classDir}")