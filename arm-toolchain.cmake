set(CMAKE_SYSTEM_NAME               iOS)

set(TOOLCHAIN_PREFIX                arm-zephyr-eabi)
find_program(CMAKE_C_COMPILER       ${TOOLCHAIN_PREFIX}-gcc)
find_program(CMAKE_CXX_COMPILER     ${TOOLCHAIN_PREFIX}-g++)

if(CMAKE_C_COMPILER STREQUAL "CMAKE_C_COMPILER-NOTFOUND")
    message(FATAL_ERROR "Could not find compiler: ${CMAKE_C_COMPILER}")
endif()

# Set architecture and CPU
set(CMAKE_SYSTEM_PROCESSOR          cortex-m33)
set(CMAKE_SYSTEM_ARCH               armv8-m.main)

unset(CMAKE_C_FLAGS_INIT)
unset(CMAKE_ASM_FLAGS_INIT)

if (DEFINED CMAKE_SYSTEM_PROCESSOR)
    set(CMAKE_C_FLAGS_INIT          "-mcpu=${CMAKE_SYSTEM_PROCESSOR}")
    set(CMAKE_ASM_FLAGS_INIT        "-mcpu=${CMAKE_SYSTEM_PROCESSOR}")
    set(CMAKE_C_LINK_FLAGS          "-mcpu=${CMAKE_SYSTEM_PROCESSOR}")
    set(CMAKE_ASM_LINK_FLAGS        "-mcpu=${CMAKE_SYSTEM_PROCESSOR}")
endif()

set(CMAKE_C_FLAGS                   ${CMAKE_C_FLAGS_INIT})
set(CMAKE_ASM_FLAGS                 ${CMAKE_ASM_FLAGS_INIT})

set(CMAKE_C_STANDARD                99)
set(CMAKE_C_STANDARD_REQUIRED       ON)
set(CMAKE_C_EXTENSIONS              OFF)

set(CMAKE_TRY_COMPILE_TARGET_TYPE   STATIC_LIBRARY)

if (NCS)
    set(ZEPHYR_SDK_VERSION          v2.1.0)
    set(ARM_ZEPHYR_TOOLCHAIN_PATH   /opt/nordic/ncs/toolchains/${ZEPHYR_SDK_VERSION}/opt/zephyr-sdk/${TOOLCHAIN_PREFIX}/bin/)
else()
    set(ZEPHYR_SDK_VERSION          0.15.1)
    set(ARM_ZEPHYR_TOOLCHAIN_PATH   /opt/toolchains/zephyr-sdk-${ZEPHYR_SDK_VERSION}/${TOOLCHAIN_PREFIX}/bin/)
endif()

set(CMAKE_C_COMPILER                ${ARM_ZEPHYR_TOOLCHAIN_PATH}/${TOOLCHAIN_PREFIX}-gcc)
set(CMAKE_ASM_COMPILER              ${ARM_ZEPHYR_TOOLCHAIN_PATH}/${TOOLCHAIN_PREFIX}-as)
set(CMAKE_CXX_COMPILER              ${ARM_ZEPHYR_TOOLCHAIN_PATH}/${TOOLCHAIN_PREFIX}-g++)
set(CMAKE_OBJCOPY                   ${ARM_ZEPHYR_TOOLCHAIN_PATH}/${TOOLCHAIN_PREFIX}-objcopy CACHE INTERNAL "objcopy tool")
set(CMAKE_SIZE_UTIL                 ${ARM_ZEPHYR_TOOLCHAIN_PATH}/${TOOLCHAIN_PREFIX}-size CACHE INTERNAL "size tool")

if (CRYPTOCELL)
    set(CMAKE_C_FLAGS                   "-DKERNEL -DNRF5340_XXAA_APPLICATION -DNRF_SKIP_FICR_NS_COPY_TO_RAM -DUSE_PARTITION_MANAGER=1 -D_FORTIFY_SOURCE=2 -D__PROGRAM_START -D__ZEPHYR__=1 -I/opt/toolchains/zephyr-sdk-0.14.1/arm-zephyr-eabi/arm-zephyr-eabi/include -I/ncs/zephyr/include/zephyr -I/ncs/zephyr/include -I/in3/build/zephyr/include/generated -I/ncs/zephyr/soc/arm/nordic_nrf/nrf53 -I/ncs/zephyr/soc/arm/nordic_nrf/common/. -I/ncs/nrf/include -I/ncs/modules/hal/cmsis/CMSIS/Core/Include -I/ncs/modules/hal/nordic/nrfx -I/ncs/modules/hal/nordic/nrfx/drivers/include -I/ncs/modules/hal/nordic/nrfx/mdk -I/ncs/zephyr/modules/hal_nordic/nrfx/. -I/in3/build/modules/nrfxlib/nrfxlib/nrf_security/src/include/generated -I/ncs/nrfxlib/nrf_security/include -I/ncs/nrfxlib/nrf_security/include/mbedtls -I/ncs/nrfxlib/nrf_security/include/psa -I/ncs/mbedtls/include -I/ncs/mbedtls/include/mbedtls -I/ncs/mbedtls/include/psa -I/ncs/mbedtls/library -I/ncs/nrfxlib/nrf_security/src/drivers/nrf_cc3xx/include -isystem /ncs/zephyr/lib/libc/minimal/include -isystem /ncs/nrfxlib/crypto/nrf_cc312_mbedcrypto/include -isystem /ncs/nrfxlib/crypto/nrf_cc312_mbedcrypto/include/mbedtls -isystem /opt/toolchains/zephyr-sdk-0.14.1/arm-zephyr-eabi/bin/../lib/gcc/arm-zephyr-eabi/10.3.0/include -isystem /opt/toolchains/zephyr-sdk-0.14.1/arm-zephyr-eabi/bin/../lib/gcc/arm-zephyr-eabi/10.3.0/include-fixed -isystem /ncs/nrfxlib/crypto/nrf_cc312_platform/include -DKERNEL -DNRF5340 -DNRF5340_XXAA_APPLICATION -D__ZEPHYR__=1 -I/ncs/zephyr/soc/arm/nordic_nrf/nrf53 -I/ncs/zephyr/soc/arm/nordic_nrf/common -I/ncs/zephyr/include -I/ncs/zephyr/include/zephyr -Os -imacros/in3/build/zephyr/include/generated/autoconf.h")
elseif(DEMO)
    set(CMAKE_C_FLAGS                   "-DKERNEL -DMBEDTLS_CONFIG_FILE=\\\"nrf-config.h\\\" -DMBEDTLS_USER_CONFIG_FILE=\\\"nrf-config-user.h\\\" -DNRF5340_XXAA_APPLICATION -DNRF_SKIP_FICR_NS_COPY_TO_RAM -DUSE_PARTITION_MANAGER=1 -D_FORTIFY_SOURCE=2 -D__PROGRAM_START -D__ZEPHYR__=1 -I/opt/toolchains/zephyr-sdk-0.14.1/arm-zephyr-eabi/arm-zephyr-eabi/include -I/key-management/cryptocell/include -I/ncs/zephyr/include/zephyr -I/ncs/zephyr/include -I/key-management/build/zephyr/include/generated -I/ncs/zephyr/soc/arm/nordic_nrf/nrf53 -I/ncs/zephyr/soc/arm/nordic_nrf/common/. -I/ncs/nrf/include -I/ncs/modules/hal/cmsis/CMSIS/Core/Include -I/ncs/modules/hal/nordic/nrfx -I/ncs/modules/hal/nordic/nrfx/drivers/include -I/ncs/modules/hal/nordic/nrfx/mdk -I/ncs/zephyr/modules/hal_nordic/nrfx/. -I/key-management/build/modules/nrfxlib/nrfxlib/nrf_security/src/include/generated -I/ncs/nrfxlib/nrf_security/include -I/ncs/nrfxlib/nrf_security/include/mbedtls -I/ncs/nrfxlib/nrf_security/include/psa -I/ncs/mbedtls/include -I/ncs/mbedtls/include/mbedtls -I/ncs/mbedtls/include/psa -I/ncs/mbedtls/library -I/ncs/nrfxlib/nrf_security/src/drivers/nrf_cc3xx/include -isystem /ncs/zephyr/lib/libc/minimal/include -isystem /ncs/nrfxlib/crypto/nrf_cc312_mbedcrypto/include -isystem /ncs/nrfxlib/crypto/nrf_cc312_mbedcrypto/include/mbedtls -isystem /opt/toolchains/zephyr-sdk-0.14.1/arm-zephyr-eabi/bin/../lib/gcc/arm-zephyr-eabi/10.3.0/include -isystem /opt/toolchains/zephyr-sdk-0.14.1/arm-zephyr-eabi/bin/../lib/gcc/arm-zephyr-eabi/10.3.0/include-fixed -isystem /ncs/nrfxlib/crypto/nrf_cc312_platform/include -DKERNEL -DNRF5340 -DNRF5340_XXAA_APPLICATION -D__ZEPHYR__=1 -I/ncs/zephyr/soc/arm/nordic_nrf/nrf53 -I/ncs/zephyr/soc/arm/nordic_nrf/common -I/ncs/zephyr/include -I/ncs/zephyr/include/zephyr -Os -imacros/key-management/build/zephyr/include/generated/autoconf.h")
endif()
set(CMAKE_CXX_FLAGS                 "${CMAKE_C_FLAGS} -fno-exceptions" CACHE INTERNAL "C++ Compiler options")

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
    --sysroot=/opt/toolchains/zephyr-sdk-0.15.1/arm-zephyr-eabi/arm-zephyr-eabi 
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
    #-fmacro-prefix-map=/in3=CMAKE_SOURCE_DIR 
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
