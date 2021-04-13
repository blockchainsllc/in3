
macro(add_static_library )
    cmake_parse_arguments(_LIB "" "NAME;REGISTER" "SOURCES;DEPENDS" ${ARGN} )

    # create objects
    add_library(${_LIB_NAME}_o OBJECT ${_LIB_SOURCES})

    # add dependency
    add_library(${_LIB_NAME} STATIC $<TARGET_OBJECTS:${_LIB_NAME}_o>)

    target_compile_definitions(${_LIB_NAME}_o PRIVATE)
    target_link_libraries(${_LIB_NAME} ${_LIB_DEPENDS})
    get_property(tmp GLOBAL PROPERTY IN3_OBJECTS)
    set_property(GLOBAL PROPERTY IN3_OBJECTS ${tmp} $<TARGET_OBJECTS:${_LIB_NAME}_o>)
    if(_LIB_REGISTER)
        get_property(tmp GLOBAL PROPERTY IN3_REGISTERS)
        set_property(GLOBAL PROPERTY IN3_REGISTERS ${tmp} ${_LIB_REGISTER})
    endif(_LIB_REGISTER)
endmacro()
