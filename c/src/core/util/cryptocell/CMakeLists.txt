if (ZEPHYR_BASE)
    zephyr_interface_library_named(cryptocell)
    zephyr_get_include_directories_for_lang_as_string(C includes STRIP_PREFIX)
    zephyr_get_compile_definitions_for_lang_as_string(C definitions)
    zephyr_get_compile_options_for_lang_as_string(C options)
    zephyr_library_include_directories(${includes})
    zephyr_library_compile_definitions(${definitions})
    zephyr_library_include_directories(
        ${PROJECT_SOURCE_DIR}/c/include/in3
        ${PROJECT_SOURCE_DIR}/c/include
    )
    zephyr_library_sources(src/cryptocell.c)
    zephyr_library_link_libraries(cryptocell core crypto init eth_basic cryptocell_signer)
endif()

