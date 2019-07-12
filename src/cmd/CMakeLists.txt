OPTION(CMD "CLI-utils" ON)
IF (CMD)
    add_subdirectory(in3)
    add_subdirectory(tools)
ENDIF (CMD)

