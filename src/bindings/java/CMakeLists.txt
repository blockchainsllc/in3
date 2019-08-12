
add_library(in3_jni SHARED in3_jni.c)
target_link_libraries(in3_jni  eth_full eth_api)

IF (NOT DEFINED ANDROID_ABI)

  find_package(Java REQUIRED)
  find_package(JNI REQUIRED)
  include(UseJava)

  project(in3j)

  include_directories(${JNI_INCLUDE_DIRS})
  set(CMAKE_JAVA_COMPILE_FLAGS "-source" "1.7" "-target" "1.7")

  add_jar(in3j 
    in3/IN3.java 
    in3/JSON.java 
    in3/StorageProvider.java 
    in3/TempStorageProvider.java 
    in3/Proof.java 
    in3/Loader.java
    in3/Signer.java

    in3/eth1/API.java
    in3/eth1/Block.java
    in3/eth1/Transaction.java
    in3/eth1/TransactionRequest.java
    in3/eth1/TransactionReceipt.java
    in3/eth1/Log.java
    in3/eth1/LogFilter.java
    in3/eth1/SimpleWallet.java

    in3/native/libin3_jni.dylib 
    in3/native/libin3_jni.so 
    in3/native/in3_jni.dll

    OUTPUT_DIR ${CMAKE_BINARY_DIR}/lib 
    OUTPUT_NAME in3
  )

ENDIF ()