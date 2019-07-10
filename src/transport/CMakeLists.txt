OPTION(TRANSPORTS "transports" ON)
IF (TRANSPORTS)
   add_subdirectory(curl)
ENDIF (TRANSPORTS)
