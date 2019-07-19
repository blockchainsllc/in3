include(${PROJECT_SOURCE_DIR}/compiler.cmake)

option(JS_ONLY "Compiles to native JS (No WASM)" OFF)

add_definitions(-std=c99 -O3 -s FETCH=1 --bind)

add_executable(a wasm.c)
target_link_libraries(a eth_full)

if (JS_ONLY)
    message(STATUS "Setting compilation target to native JavaScript")
    set(CMAKE_EXECUTABLE_SUFFIX ".js")
    set_target_properties(a PROPERTIES LINK_FLAGS "-s EXPORTED_FUNCTIONS='[_main]'")
else (JS_ONLY)
    message(STATUS "Setting compilation target to WASM")
    set(CMAKE_EXECUTABLE_SUFFIX ".wasm.js")
    set_target_properties(a PROPERTIES LINK_FLAGS "-s WASM=1 -s BINARYEN_METHOD='native-wasm' -s EXPORTED_FUNCTIONS='[_main]'")
endif (JS_ONLY)