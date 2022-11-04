set(TOOLCHAIN_PREFIX arm-zephyr-eabi)
find_program(CMAKE_C_COMPILER ${TOOLCHAIN_PREFIX}-gcc)
find_program(CMAKE_CXX_COMPILER ${TOOLCHAIN_PREFIX}-g++)

if(CMAKE_C_COMPILER STREQUAL "CMAKE_C_COMPILER-NOTFOUND")
    message(FATAL_ERROR "Could not find compiler: ${CMAKE_C_COMPILER}")
endif()

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

if(NCS)
    set(ZEPHYR_SDK_VERSION v2.1.0)
    set(ARM_ZEPHYR_TOOLCHAIN_PATH /opt/nordic/ncs/toolchains/${ZEPHYR_SDK_VERSION}/opt/zephyr-sdk/${TOOLCHAIN_PREFIX}/bin/)
else()
    set(ZEPHYR_SDK_VERSION 0.15.1)
    set(ARM_ZEPHYR_TOOLCHAIN_PATH /opt/toolchains/zephyr-sdk-${ZEPHYR_SDK_VERSION}/${TOOLCHAIN_PREFIX}/bin/)
endif()

set(CMAKE_C_COMPILER ${ARM_ZEPHYR_TOOLCHAIN_PATH}/${TOOLCHAIN_PREFIX}-gcc)
set(CMAKE_ASM_COMPILER ${ARM_ZEPHYR_TOOLCHAIN_PATH}/${TOOLCHAIN_PREFIX}-as)
set(CMAKE_CXX_COMPILER ${ARM_ZEPHYR_TOOLCHAIN_PATH}/${TOOLCHAIN_PREFIX}-g++)
set(CMAKE_OBJCOPY ${ARM_ZEPHYR_TOOLCHAIN_PATH}/${TOOLCHAIN_PREFIX}-objcopy CACHE INTERNAL "objcopy tool")
set(CMAKE_SIZE_UTIL ${ARM_ZEPHYR_TOOLCHAIN_PATH}/${TOOLCHAIN_PREFIX}-size CACHE INTERNAL "size tool")

set(CMAKE_C_FLAGS "-I/opt/toolchains/zephyr-sdk-0.14.1/arm-zephyr-eabi/arm-zephyr-eabi/include -I/ncs/zephyr/include/zephyr -I/ncs/zephyr/include -I${CMAKE_CURRENT_BINARY_DIR}/zephyr/include/generated -I/ncs/zephyr/soc/arm/nordic_nrf/nrf53 -I/ncs/zephyr/soc/arm/nordic_nrf/common/. -I/ncs/nrf/include -I/ncs/modules/hal/cmsis/CMSIS/Core/Include -I/ncs/modules/hal/nordic/nrfx -I/ncs/modules/hal/nordic/nrfx/drivers/include -I/ncs/modules/hal/nordic/nrfx/mdk -I/ncs/zephyr/modules/hal_nordic/nrfx/. -I${CMAKE_CURRENT_BINARY_DIR}/modules/nrfxlib/nrfxlib/nrf_security/src/include/generated -I/ncs/nrfxlib/nrf_security/include -I/ncs/nrfxlib/nrf_security/include/mbedtls -I/ncs/nrfxlib/nrf_security/include/psa -I/ncs/mbedtls/include -I/ncs/mbedtls/include/mbedtls -I/ncs/mbedtls/include/psa -I/ncs/mbedtls/library -I/ncs/nrfxlib/nrf_security/src/drivers/nrf_cc3xx/include -isystem /ncs/zephyr/lib/libc/minimal/include -isystem /ncs/nrfxlib/crypto/nrf_cc312_mbedcrypto/include -isystem /ncs/nrfxlib/crypto/nrf_cc312_mbedcrypto/include/mbedtls -isystem /opt/toolchains/zephyr-sdk-0.14.1/arm-zephyr-eabi/bin/../lib/gcc/arm-zephyr-eabi/10.3.0/include -isystem /opt/toolchains/zephyr-sdk-0.14.1/arm-zephyr-eabi/bin/../lib/gcc/arm-zephyr-eabi/10.3.0/include-fixed -isystem /ncs/nrfxlib/crypto/nrf_cc312_platform/include -DKERNEL -DNRF5340 -DNRF5340_XXAA_APPLICATION -D__ZEPHYR__=1 -I/ncs/zephyr/soc/arm/nordic_nrf/nrf53 -I/ncs/zephyr/soc/arm/nordic_nrf/common -I/ncs/zephyr/include -I/ncs/zephyr/include/zephyr -Os -imacros${CMAKE_CURRENT_BINARY_DIR}/zephyr/include/generated/autoconf.h")

set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} -fno-exceptions" CACHE INTERNAL "C++ Compiler options")

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

add_compile_options(
    -ffreestanding
    -fno-common
    -g
    -gdwarf-4
    -fdiagnostics-color=always
    -mcpu=cortex-m33
    -mthumb
    -mabi=aapcs
    -mfpu=fpv5-sp-d16
    -mfloat-abi=hard
    -mfp16-format=ieee
    --sysroot=/opt/toolchains/zephyr-sdk-0.14.1/arm-zephyr-eabi/arm-zephyr-eabi
    -imacros /ncs/zephyr/include/zephyr/toolchain/zephyr_stdint.h
    -Wall
    -Wformat
    -Wformat-security
    -Wno-format-zero-length
    -Wno-main
    -Wno-pointer-sign
    -Wpointer-arith
    -Wexpansion-to-defined
    -Wno-unused-but-set-variable
    -Werror=implicit-int
    -fno-asynchronous-unwind-tables
    -fno-pie
    -fno-pic
    -fno-reorder-functions
    -fno-defer-pop
    -fmacro-prefix-map=/key-management=CMAKE_SOURCE_DIR
    -fmacro-prefix-map=/ncs/zephyr=ZEPHYR_BASE
    -fmacro-prefix-map=/ncs=WEST_TOPDIR
    -ffunction-sections
    -fdata-sections
    -mcmse
    -std=c99
)

add_link_options(
    -mcpu=cortex-m33
    -mthumb
    -mabi=aapcs
    -mfpu=fpv5-sp-d16
    -mfloat-abi=hard
    -mfp16-format=ieee
)
