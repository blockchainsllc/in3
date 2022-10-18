if (ZEPHYR_BASE)
  add_library(cryptocell STATIC src/cryptocell.c)
  add_dependencies(cryptocell zephyr_interface core)
endif()