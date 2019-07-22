add_library(usn_api_o OBJECT usn_api.c)
add_library(usn_api STATIC $<TARGET_OBJECTS:usn_api_o>)
target_link_libraries(usn_api core)