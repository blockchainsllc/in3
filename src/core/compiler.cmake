set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC -Wall -funsigned-char -Wextra  -std=c99 -D_POSIX_C_SOURCE=199309L")
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC -Wall -funsigned-char -Wextra  -std=c99 -D__FILENAME__='\"$(subst ${CMAKE_SOURCE_DIR}/,,$(abspath $<))\"'")
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set_property(GLOBAL PROPERTY C_STANDARD 99)

# Set DEAD_STRIP_LINKER_OPT
if(NOT CYGWIN AND NOT MINGW)
  if(NOT ${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    SET(CMAKE_CXX_FLAGS
            "${CMAKE_CXX_FLAGS}  -ffunction-sections -fdata-sections"
            PARENT_SCOPE)
  endif()
endif()
if(NOT LLVM_NO_DEAD_STRIP)
  if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    set_property(GLOBAL PROPERTY DEAD_STRIP_LINKER_OPT "-Wl,-dead_strip")
  elseif(NOT WIN32)
    set_property(GLOBAL PROPERTY DEAD_STRIP_LINKER_OPT "-Wl,--gc-sections")
  endif()
endif()

# Only CMake >=3.13 has add_link_options(). For previous versions,
# DEAD_STRIP_LINKER_OPT can be used with target_link_libraries()
if(${CMAKE_VERSION} VERSION_GREATER "3.12.4")
  get_property(DEAD_STRIP_LINKER_OPT GLOBAL PROPERTY DEAD_STRIP_LINKER_OPT)
  add_link_options(${DEAD_STRIP_LINKER_OPT})
  set_property(GLOBAL PROPERTY DEAD_STRIP_LINKER_OPT "")
endif()

OPTION(ZEPHYR "ZEPHYR" OFF)
IF(ZEPHYR)
  add_definitions(-DZEPHYR_OS)
  add_definitions(-DZEPHYR)
ENDIF(ZEPHYR)
