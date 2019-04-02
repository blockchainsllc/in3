set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC -Wall -funsigned-char -Wextra  -std=c99 ")
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC -Wall -funsigned-char -Wextra  -std=c99 -D__FILENAME__='\"$(subst ${CMAKE_SOURCE_DIR}/,,$(abspath $<))\"'")
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set_property(GLOBAL PROPERTY C_STANDARD 99)

function(add_dead_strip)
  if(NOT CYGWIN AND NOT MINGW)
    if(NOT ${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
      SET(CMAKE_CXX_FLAGS
              "${CMAKE_CXX_FLAGS}  -ffunction-sections -fdata-sections"
              PARENT_SCOPE)
    endif()
  endif()
  if(NOT LLVM_NO_DEAD_STRIP)
    if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
      add_link_options(-Wl,-dead_strip)
    elseif(NOT WIN32)
      add_link_options(-Wl,--gc-sections)
    endif()
  endif()
endfunction(add_dead_strip)

add_dead_strip()

OPTION(ZEPHYR "ZEPHYR" OFF)
IF(ZEPHYR)
  add_definitions(-DZEPHYR_OS)
  add_definitions(-DZEPHYR)
ENDIF(ZEPHYR)
