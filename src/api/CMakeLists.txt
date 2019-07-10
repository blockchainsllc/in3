
OPTION(IN3API "apis" ON)
IF (IN3API)
  add_subdirectory(eth1)
  add_subdirectory(usn)
ENDIF (IN3API)

