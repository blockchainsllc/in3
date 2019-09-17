# main incubed module defining the interfaces for transport, verifier and storage. 
#
# This module does not have any dependencies and cannot be used without additional modules providing verification and transport.
OPTION(IN3_STAGING "if true, the client will use the staging-network instead of the live ones" ON)
IF (IN3_STAGING)
   ADD_DEFINITIONS(-DIN3_STAGING)
ENDIF(IN3_STAGING)

add_library(core_o OBJECT
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
add_library(core STATIC $<TARGET_OBJECTS:core_o>)
target_link_libraries(core tommath crypto)
