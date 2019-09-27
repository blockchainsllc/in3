###############################################################################
# This file is part of the Incubed project.
# Sources: https://github.com/slockit/in3-c
# 
# Copyright (C) 2018-2019 slock.it GmbH, Blockchains LLC
# 
# 
# COMMERCIAL LICENSE USAGE
# 
# Licensees holding a valid commercial license may use this file in accordance 
# with the commercial license agreement provided with the Software or, alternatively, 
# in accordance with the terms contained in a written agreement between you and 
# slock.it GmbH/Blockchains LLC. For licensing terms and conditions or further 
# information please contact slock.it at in3@slock.it.
# 	
# Alternatively, this file may be used under the AGPL license as follows:
#    
# AGPL LICENSE USAGE
# 
# This program is free software: you can redistribute it and/or modify it under the
# terms of the GNU Affero General Public License as published by the Free Software 
# Foundation, either version 3 of the License, or (at your option) any later version.
#  
# This program is distributed in the hope that it will be useful, but WITHOUT ANY 
# WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A 
# PARTICULAR PURPOSE. See the GNU Affero General Public License for more details.
# [Permissions of this strong copyleft license are conditioned on making available 
# complete source code of licensed works and modifications, which include larger 
# works using a licensed work, under the same license. Copyright and license notices 
# must be preserved. Contributors provide an express grant of patent rights.]
# You should have received a copy of the GNU Affero General Public License along 
# with this program. If not, see <https://www.gnu.org/licenses/>.
###############################################################################


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
    in3/native/libin3_jni_arm.so
    in3/native/in3_jni.dll

    OUTPUT_DIR ${CMAKE_BINARY_DIR}/lib 
    OUTPUT_NAME in3
  )

ENDIF ()