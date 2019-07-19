
OPTION(IN3API "build the USN-API which offer better interfaces and additional functions on top of the pure verification" ON)
IF (IN3API)
    add_subdirectory(eth1)
    add_subdirectory(usn)
ENDIF (IN3API)

