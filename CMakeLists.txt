cmake_minimum_required(VERSION 3.20)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

set(CMAKE_SYSTEM_NAME "Generic")
set(CMAKE_C_COMPILER "arm-none-eabi-gcc")
set(CMAKE_CXX_COMPILER "arm-none-eabi-g++")
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)
set(CMAKE_OBJCOPY "arm-none-eabi-objcopy")

file(GLOB_RECURSE STM32WL_HAL_SRC
        "external/stm32wl_hal/Src/stm32wlxx_ll*.c"
        "external/stm32wl_hal/Inc/stm32wlxx_ll*.h"
        "external/stm32wl_cmsis/Include/*.h"
)

project(on9ldr C CXX ASM)


add_executable(
        on9ldr
        "main/stm32wlxx_it.c" "main/stm32wlxx_it.h"
        "main/startup_stm32wle5xx.s" "main/system_stm32wlxx.c"
        ${STM32WL_HAL_SRC}
        "main/hal/clock_setup.c" "main/hal/clock_setup.h"
        "main/main.cpp" "main/misc/misc.hpp"
        "external/printf/src/printf/printf.c"
        "main/misc/log.c" "main/misc/log.h"
)

target_include_directories(on9ldr PUBLIC
        "external/stm32wl_cmsis/Include"
        "external/stm32wl_hal/Inc"
        "external/printf/src/printf" "external/printf/src"
        "external/cmsis/Include"
        "main" "main/misc"
)

if (CMAKE_BUILD_TYPE MATCHES RelWithDebInfo)
    message("Build in Release+Log mode")
    target_compile_options(on9ldr PUBLIC
            -ffunction-sections -fdata-sections -flto -Os
            -fno-rtti -fno-asynchronous-unwind-tables -fno-omit-frame-pointer -fno-exceptions
            -mthumb -mcpu=cortex-m4 -mfloat-abi=soft
    )

    target_link_options(on9ldr PUBLIC
            -T${CMAKE_CURRENT_LIST_DIR}/misc/stm32wle5.ld
            -Wl,-Map=\"${PROJECT_NAME}.map\"
            -lc -flto -Os
            -Wl,--gc-sections -specs=nano.specs -specs=nosys.specs
            -mthumb -mcpu=cortex-m4 -mfloat-abi=soft
    )

    target_compile_definitions(on9ldr PUBLIC
            CORE_CM4=1
            STM32WLE5xx=1
            USE_FULL_LL_DRIVER=1
    )
elseif(CMAKE_BUILD_TYPE MATCHES Release)
    message("Build in Release with no log mode")
    target_compile_options(on9ldr PUBLIC
            -ffunction-sections -fdata-sections -flto -Os
            -fno-rtti -fno-asynchronous-unwind-tables -fno-omit-frame-pointer -fno-exceptions
            -mthumb -mcpu=cortex-m4 -mfloat-abi=soft
    )

    target_link_options(on9ldr PUBLIC
            -T${CMAKE_CURRENT_LIST_DIR}/misc/stm32wle5.ld
            -Wl,-Map=\"${PROJECT_NAME}.map\"
            -Wl,--gc-sections -flto -Os -specs=nano.specs -specs=nosys.specs
            -mthumb -mcpu=cortex-m4 -mfloat-abi=soft
    )

    target_compile_definitions(on9ldr PUBLIC
            CORE_CM4=1
            STM32WLE5xx=1
            USE_FULL_LL_DRIVER=1
            DISABLE_LOG=1
    )
else()
    message("Unknown state, debug mode")
    target_compile_options(on9ldr PUBLIC
            -Og
            -ffunction-sections -fdata-sections
            -fno-rtti -fno-asynchronous-unwind-tables -fno-omit-frame-pointer -fno-exceptions
            -mthumb -mcpu=cortex-m4 -mfloat-abi=soft
    )

    target_link_options(on9ldr PUBLIC
            -Og
            -T${CMAKE_CURRENT_LIST_DIR}/misc/stm32wle5.ld
            -Wl,-Map=\"${PROJECT_NAME}.map\"
            -lc
            -Wl,--gc-sections -specs=nano.specs -specs=nosys.specs
            -mthumb -mcpu=cortex-m4 -mfloat-abi=soft
    )

    target_compile_definitions(on9ldr PUBLIC
            CORE_CM4=1
            STM32WLE5xx=1
            USE_FULL_LL_DRIVER=1
    )
endif()

if (STM32WL_HAS_TCXO MATCHES 1)
    message(STATUS "on9loader: TCXO enabled")
    target_compile_definitions(on9ldr PUBLIC STM32WL_HAS_TCXO=1)
elseif()
    message(STATUS "on9loader: TCXO disabled")
endif()

set(linker_script "misc/stm32wle5.ld")
set_target_properties(on9ldr PROPERTIES SUFFIX ".elf")
add_custom_command(TARGET on9ldr POST_BUILD
        COMMAND arm-none-eabi-size --format=berkeley --totals "$<TARGET_FILE:on9ldr>"
        COMMAND ${CMAKE_OBJCOPY} -Oihex "$<TARGET_FILE:on9ldr>" firmware.hex
        COMMAND ${CMAKE_OBJCOPY} -Obinary "$<TARGET_FILE:on9ldr>" firmware.bin
        COMMENT "Invoking: Cross ARM GNU Print Size")