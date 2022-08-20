
macro(add_static_library)
    cmake_parse_arguments(_LIB "" "NAME;REGISTER;OPTION;DESCR;TYPE" "SOURCES;DEPENDS" ${ARGN})

    if(_LIB_OPTION)
        option(${_LIB_OPTION} ${_LIB_DESCR} ON)
    else()
        string(TOUPPER "MOD_${_LIB_NAME}" _LIB_OPTION)
        set(${_LIB_OPTION} ON)
    endif()

    if(${${_LIB_OPTION}})
        # create objects
        add_library(${_LIB_NAME}_o OBJECT ${_LIB_SOURCES})

        # add dependency
        add_library(${_LIB_NAME} STATIC $<TARGET_OBJECTS:${_LIB_NAME}_o>)

        target_compile_definitions(${_LIB_NAME}_o PRIVATE)
        target_link_libraries(${_LIB_NAME} ${_LIB_DEPENDS})
        get_property(RPC_YML_TMP GLOBAL PROPERTY RPC_YML)
        get_property(tmp GLOBAL PROPERTY IN3_OBJECTS)
        set_property(GLOBAL PROPERTY IN3_OBJECTS ${tmp} $<TARGET_OBJECTS:${_LIB_NAME}_o>)
        get_property(tmp GLOBAL PROPERTY IN3_API_CONF)
        set_property(GLOBAL PROPERTY IN3_API_CONF ${tmp} _api ${_LIB_NAME} _descr ${_LIB_DESCR} _dep ${_LIB_DEPENDS} _type ${_LIB_TYPE} _register ${_LIB_REGISTER} _dir ${CMAKE_CURRENT_SOURCE_DIR})
        get_property(tmp GLOBAL PROPERTY IN3_API_NAMES)
        set_property(GLOBAL PROPERTY IN3_API_NAMES ${tmp} ${_LIB_NAME})
        set_property(GLOBAL PROPERTY RPC_YML ${RPC_YML_TMP} ${CMAKE_CURRENT_SOURCE_DIR})

        if(_LIB_REGISTER)
            get_property(tmp GLOBAL PROPERTY IN3_REGISTERS)
            set_property(GLOBAL PROPERTY IN3_REGISTERS ${tmp} ${_LIB_REGISTER})
        endif(_LIB_REGISTER)
    endif()
endmacro()
