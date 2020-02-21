# first we can indicate the documentation build as an option and set it to ON by default
if (BUILD_DOC)
  # check if Doxygen is installed
  find_package(Doxygen)
  if (DOXYGEN_FOUND)
      # set input and output files
      set(DOXYGEN_IN_JAVA ${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in)
      set(DOXYGEN_OUT_JAVA ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)

      # request to configure the file
      configure_file(${DOXYGEN_IN_JAVA} ${DOXYGEN_OUT_JAVA} @ONLY)
      message("Doxygen java build started")

      # note the option ALL which allows to build the docs together with the application
      add_custom_target( doc_doxygen_java ALL
          COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYGEN_OUT_JAVA}
          WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
          COMMENT "Generating Java API documentation with Doxygen"
          VERBATIM )
  else (DOXYGEN_FOUND)
    message("Doxygen need to be installed to generate the doxygen documentation")
  endif (DOXYGEN_FOUND)
endif()