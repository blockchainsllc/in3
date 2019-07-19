OPTION(JAVA "build the java-binding (shared-lib and jar-file)" OFF)
IF (JAVA)
    add_subdirectory(java)
ENDIF (JAVA)

OPTION(WASM "Includes the WASM-Build. In order to build it you need emscripten as toolchain. Usually you also want to turn off other builds in this case." OFF)
IF (WASM)
    add_subdirectory(wasm)
ENDIF (WASM)
