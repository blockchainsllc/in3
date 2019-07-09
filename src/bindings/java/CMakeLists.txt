find_package(Java REQUIRED)
include(UseJava)

project (in3j)

#set(CMAKE_JAVA_COMPILE_FLAGS "-source" "1.6" "-target" "1.6")

add_jar(in3j in3/IN3.java in3/StorageProvider.java in3/Proof.java)

#get_target_property(_jarFile in3j JAR_FILE)
#get_target_property(_classDir in3j CLASSDIR)

message(STATUS "Jar file ${_jarFile}")
message(STATUS "Class compiled to ${_classDir}")