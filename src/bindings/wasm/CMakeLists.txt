include(${PROJECT_SOURCE_DIR}/compiler.cmake)

option(JS_ONLY "Compiles to native JS (No WASM)" OFF)
add_executable(in3w wasm.c)
target_link_libraries(in3w eth_full)

if (JS_ONLY)
    message(STATUS "Setting compilation target to native JavaScript")
    set(CMAKE_EXECUTABLE_SUFFIX ".js")
    set_target_properties(in3w PROPERTIES LINK_FLAGS "-s EXPORTED_FUNCTIONS='[_main,_in3_create,_in3_send, _in3_dispose]'")
else (JS_ONLY)
    message(STATUS "Setting compilation target to WASM")
    set_target_properties(in3w PROPERTIES LINK_FLAGS "-s EXPORT_NAME=in3w -s WASM=1 -s ASYNCIFY=1 -O3 -s 'ASYNCIFY_IMPORTS=[\"transport_send\"]' -s 'EXTRA_EXPORTED_RUNTIME_METHODS=[\"ccall\", \"cwrap\"]'")
endif (JS_ONLY)
