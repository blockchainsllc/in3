# main incubed module defining the interfaces for transport, verifier and storage. 
#
# This module does not have any dependencies and cannot be used without additional modules providing verification and transport.


add_library(core STATIC
        client/context.c
        client/client.c
        client/cache.c
        client/nodelist.c
        client/verifier.c
        client/send.c
        client/client_init.c
        util/debug.c
        util/bytes.c
        util/utils.c
        util/scache.c
        util/data.c
        util/log.c
        util/mem.c
        util/stringbuilder.c
        util/bitset.c
        )
include("${PROJECT_SOURCE_DIR}/compiler.cmake")
target_include_directories(core PUBLIC "${PROJECT_SOURCE_DIR}/src/third-party")
