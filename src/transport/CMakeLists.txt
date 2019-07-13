OPTION(TRANSPORTS "builds transports, which may require extra libraries." ON)
IF (TRANSPORTS)
    add_subdirectory(curl)
ENDIF (TRANSPORTS)
