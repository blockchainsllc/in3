OPTION(JAVA "Java" OFF)
IF (JAVA)
    add_subdirectory(java)
ENDIF (JAVA)

OPTION(WASM "Wasm" OFF)
IF (WASM)
    add_subdirectory(wasm)
ENDIF (WASM)
