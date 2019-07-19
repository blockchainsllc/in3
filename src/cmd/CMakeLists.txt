OPTION(CMD "build the comandline utils" ON)
IF (CMD)
    add_subdirectory(in3)
    add_subdirectory(tools)
ENDIF (CMD)

