
# import results from conan, which declares the dependend libraries.
include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup(TARGETS)

# static lib
add_library( in3_curl STATIC in3_curl.c in3_storage.c)

# add dependency
target_link_libraries(in3_curl CONAN_PKG::libcurl)

# props
include(../core/compiler.cmake)
