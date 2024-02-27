 # ================================================================
 # Author   : German Mundinger
 # Date     : 2023
 # ================================================================

# cmake -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTS=ON ..

cmake_minimum_required(VERSION 3.22)

option(BUILD_TESTS "Enable tests building." OFF)
if(BUILD_TESTS)
    set(CMAKE_TOOLCHAIN_FILE "${CMAKE_CURRENT_SOURCE_DIR}/cmake/amd64.cmake")
else()
    set(CMAKE_TOOLCHAIN_FILE "${CMAKE_CURRENT_SOURCE_DIR}/cmake/arm.cmake")
endif()

# Project name
project(blackpill_node LANGUAGES C ASM VERSION 1.0.0.0)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

include(FetchContent)

if(BUILD_TESTS)
    enable_language(CXX)
    enable_testing()
    add_subdirectory(tests)
endif()

FetchContent_Declare(
    common_code
    SOURCE_DIR      ${PROJECT_SOURCE_DIR}/external/common_code
    GIT_REPOSITORY  https://github.com/germandevelop/common.git
    GIT_TAG         main
)
FetchContent_GetProperties(common_code)
if(NOT common_code_POPULATED)
    FetchContent_Populate(common_code)
endif()

FetchContent_Declare(
    arm_cmsis
    SOURCE_DIR      ${PROJECT_SOURCE_DIR}/external/arm_cmsis
    GIT_REPOSITORY  https://github.com/ARM-software/CMSIS_6.git
    GIT_TAG         v6.0.0
)
FetchContent_GetProperties(arm_cmsis)
if(NOT arm_cmsis_POPULATED)
    FetchContent_Populate(arm_cmsis)
endif()

FetchContent_Declare(
    stm32_device
    SOURCE_DIR      ${PROJECT_SOURCE_DIR}/external/stm32_device
    GIT_REPOSITORY  https://github.com/STMicroelectronics/cmsis_device_f4.git
    GIT_TAG         v2.6.9
)
FetchContent_GetProperties(stm32_device)
if(NOT stm32_device_POPULATED)
    FetchContent_Populate(stm32_device)
endif()

FetchContent_Declare(
    stm32_hal
    SOURCE_DIR      ${PROJECT_SOURCE_DIR}/external/stm32_hal
    GIT_REPOSITORY  https://github.com/STMicroelectronics/stm32f4xx_hal_driver.git
    GIT_TAG         v1.8.2
)
FetchContent_GetProperties(stm32_hal)
if(NOT stm32_hal_POPULATED)
    FetchContent_Populate(stm32_hal)
endif()

FetchContent_Declare(
    free_rtos
    SOURCE_DIR      ${PROJECT_SOURCE_DIR}/external/free_rtos
    GIT_REPOSITORY  https://github.com/FreeRTOS/FreeRTOS-Kernel.git
    GIT_TAG         V11.0.1
)
FetchContent_GetProperties(free_rtos)
if(NOT free_rtos_POPULATED)
    FetchContent_Populate(free_rtos)
endif()

FetchContent_Declare(
    w5500_driver
    SOURCE_DIR      ${PROJECT_SOURCE_DIR}/external/w5500_driver
    GIT_REPOSITORY  https://github.com/Wiznet/ioLibrary_Driver.git
    GIT_TAG         v3.1.3
)
FetchContent_GetProperties(w5500_driver)
if(NOT w5500_driver_POPULATED)
    FetchContent_Populate(w5500_driver)
endif()

FetchContent_Declare(
    little_fs
    SOURCE_DIR      ${PROJECT_SOURCE_DIR}/external/little_fs
    GIT_REPOSITORY  https://github.com/littlefs-project/littlefs.git
    GIT_TAG         v2.8.2
)
FetchContent_GetProperties(little_fs)
if(NOT little_fs_POPULATED)
    FetchContent_Populate(little_fs)
endif()

FetchContent_Declare(
    bme280_driver
    SOURCE_DIR      ${PROJECT_SOURCE_DIR}/external/bme280_driver
    GIT_REPOSITORY  https://github.com/boschsensortec/BME280_driver.git
    GIT_TAG         bme280_v3.5.0
)
FetchContent_GetProperties(bme280_driver)
if(NOT bme280_driver_POPULATED)
    FetchContent_Populate(bme280_driver)
endif()

FetchContent_Declare(
    lwjson_parser
    SOURCE_DIR      ${PROJECT_SOURCE_DIR}/external/lwjson_parser
    GIT_REPOSITORY  https://github.com/MaJerle/lwjson.git
    GIT_TAG         v1.6.1
)
FetchContent_GetProperties(lwjson_parser)
if(NOT lwjson_parser_POPULATED)
    FetchContent_Populate(lwjson_parser)
endif()



add_library(blackpill_config INTERFACE)
target_include_directories(blackpill_config
    INTERFACE
        src
)
target_compile_features(blackpill_config
    INTERFACE
        c_std_17
)
target_compile_definitions(blackpill_config
    INTERFACE
        $<$<CONFIG:Debug>:USE_FULL_ASSERT>
        $<$<CONFIG:Release>:NDEBUG>
)
target_compile_options(blackpill_config
    INTERFACE
        -Wall
        -Wextra
        -pedantic
        $<$<CONFIG:Debug>:-Og>
        $<$<CONFIG:Debug>:-g2>
        $<$<CONFIG:Release>:-O3>
        $<$<CONFIG:Release>:-g0>
)


add_subdirectory(external/common_code)

# Create general blackpill bootloader target
add_library(blackpill_bootloader INTERFACE)
target_link_libraries(blackpill_bootloader INTERFACE blackpill_config)
target_link_libraries(blackpill_bootloader INTERFACE std_error)
target_include_directories(blackpill_bootloader
    INTERFACE
        external/arm_cmsis/CMSIS/Core/Include
        external/stm32_device/Include
        external/stm32_hal/Inc
        external/little_fs
)
target_sources(blackpill_bootloader
    INTERFACE
        src/devices/w25q32bv_flash.h
        src/devices/w25q32bv_flash.c
        src/board.uart_2.h
        src/board.uart_2.c
        src/logger.h
        src/logger.c

        src/lfs_config.h
        src/lfs_config.c
        src/stm32f4xx_hal_conf.h
        src/stm32f4xx_hal_debug.c
        src/stm32f4xx_hal_msp.c
        src/stm32f4xx_it.h
        src/stm32f4xx_it.c

        external/stm32_device/Source/Templates/system_stm32f4xx.c

        external/stm32_hal/Src/stm32f4xx_hal.c
        external/stm32_hal/Src/stm32f4xx_hal_cortex.c
        external/stm32_hal/Src/stm32f4xx_hal_rcc.c
        external/stm32_hal/Src/stm32f4xx_hal_rcc_ex.c
        external/stm32_hal/Src/stm32f4xx_hal_gpio.c
        external/stm32_hal/Src/stm32f4xx_hal_uart.c
        external/stm32_hal/Src/stm32f4xx_hal_spi.c

        external/little_fs/lfs_util.h
        external/little_fs/lfs.h
        external/little_fs/lfs.c
)
target_compile_definitions(blackpill_bootloader
    INTERFACE
        USE_HAL_DRIVER
        LFS_CONFIG=lfs_config.h
)
target_compile_options(blackpill_bootloader
    INTERFACE
        -mcpu=cortex-m4
        -mthumb
        -mfpu=fpv4-sp-d16
        -mfloat-abi=hard
        -fdata-sections
        -ffunction-sections
)
target_link_options(blackpill_bootloader
    INTERFACE
        -mcpu=cortex-m4
        -mthumb
        -mfpu=fpv4-sp-d16
        -mfloat-abi=hard
        -specs=nano.specs
        -specs=nosys.specs
        -u _printf_float
        -lc
        -lm
        #-lnosys
        -Wl,-Map=${PROJECT_NAME}.map,--cref
        -Wl,--gc-sections
        -Wl,--print-memory-usage
)

# Create blackpill under testing target
add_library(blackpill_under_testing INTERFACE)
target_link_libraries(blackpill_under_testing INTERFACE blackpill_config)
target_link_libraries(blackpill_under_testing INTERFACE std_error)
target_sources(blackpill_under_testing
    INTERFACE
        src/devices/mcp23017_expander.h
        src/devices/mcp23017_expander.c
        src/node_T01.h
        src/node_T01.c
)

# Create blackpill firmware target
add_library(blackpill_firmware INTERFACE)
target_link_libraries(blackpill_firmware INTERFACE blackpill_bootloader)
target_link_libraries(blackpill_firmware INTERFACE blackpill_under_testing)
target_link_libraries(blackpill_firmware INTERFACE vs1838_control)
target_include_directories(blackpill_firmware
    INTERFACE
        external/free_rtos/include
        external/free_rtos/portable/GCC/ARM_CM4F
        external/w5500_driver/Ethernet
        external/lwjson_parser/lwjson/src/include
        external/bme280_driver
)
target_sources(blackpill_firmware
    INTERFACE
        src/board.h
        src/board.c
        src/board.types.h
        src/board.uart_2.h
        src/board.uart_2.c
        src/board.spi_1.h
        src/board.spi_1.c
        src/board.i2c_1.h
        src/board.i2c_1.c
        src/board.adc_1.h
        src/board.adc_1.c
        src/board.exti_1.h
        src/board.exti_1.c
        src/board.exti_2.h
        src/board.exti_2.c
        src/board.timer_2.h
        src/board.timer_2.c
        src/board.timer_3.h
        src/board.timer_3.c
        src/node.h
        src/node.c
        src/node.mapper.h
        src/node.mapper.c
        src/tcp_client.h
        src/tcp_client.c
        src/tcp_client.types.h

        src/board_T01.h
        src/board_T01.c
        src/node_T01.h
        src/node_T01.c

        src/devices/ssd1306_display.h
        src/devices/ssd1306_display.c
        src/devices/bme280_sensor.h
        src/devices/bme280_sensor.c

        src/FreeRTOSConfig.h
        src/lwjson_opts.h

        src/stm32f4xx_it.h
        src/stm32f4xx_it.c
        src/stm32f4xx_hal_it_timer11.c

        external/stm32_hal/Src/stm32f4xx_hal_tim.c
        external/stm32_hal/Src/stm32f4xx_hal_tim_ex.c
        external/stm32_hal/Src/stm32f4xx_hal_adc.c
        external/stm32_hal/Src/stm32f4xx_hal_adc_ex.c
        external/stm32_hal/Src/stm32f4xx_hal_dma_ex.c
        external/stm32_hal/Src/stm32f4xx_hal_dma.c
        external/stm32_hal/Src/stm32f4xx_hal_pwr.c
        external/stm32_hal/Src/stm32f4xx_hal_pwr_ex.c
        external/stm32_hal/Src/stm32f4xx_hal_exti.c
        external/stm32_hal/Src/stm32f4xx_hal_i2c.c
        external/stm32_hal/Src/stm32f4xx_hal_i2c_ex.c
        external/stm32_hal/Src/stm32f4xx_hal_iwdg.c
        external/stm32_hal/Src/stm32f4xx_hal_flash.c
        external/stm32_hal/Src/stm32f4xx_hal_flash_ex.c
        external/stm32_hal/Src/stm32f4xx_hal_flash_ramfunc.c

        external/free_rtos/tasks.c
        external/free_rtos/list.c
        external/free_rtos/queue.c
        external/free_rtos/timers.c
        external/free_rtos/portable/MemMang/heap_4.c
        external/free_rtos/portable/GCC/ARM_CM4F/port.c

        external/w5500_driver/Ethernet/W5500/w5500.c
        external/w5500_driver/Ethernet/socket.c
        external/w5500_driver/Ethernet/wizchip_conf.c

        external/lwjson_parser/lwjson/src/lwjson/lwjson.c

        external/bme280_driver/bme280_defs.h
        external/bme280_driver/bme280.h
        external/bme280_driver/bme280.c
)
target_compile_definitions(blackpill_firmware
    INTERFACE
        _WIZCHIP_=5500
        BME280_FLOAT_ENABLE
)

set_source_files_properties(external/w5500_driver/Ethernet/socket.c
    PROPERTIES COMPILE_OPTIONS "-Wno-parentheses")
set_source_files_properties(external/w5500_driver/Ethernet/wizchip_conf.c
    PROPERTIES COMPILE_OPTIONS "-Wno-missing-braces;-Wno-unused-parameter")
set_source_files_properties(external/stm32_hal/Src/stm32f4xx_hal_exti.c
    PROPERTIES COMPILE_OPTIONS "-Wno-unused-parameter")
set_source_files_properties(external/stm32_hal/Src/stm32f4xx_hal_cortex.c
    PROPERTIES COMPILE_OPTIONS "-Wno-unused-parameter")
set_source_files_properties(external/stm32_hal/Src/stm32f4xx_hal_flash_ex.c
    PROPERTIES COMPILE_OPTIONS "-Wno-unused-parameter")
set_source_files_properties(external/stm32_hal/Src/stm32f4xx_hal_pwr.c
    PROPERTIES COMPILE_OPTIONS "-Wno-unused-parameter")
set_source_files_properties(external/stm32_hal/Src/stm32f4xx_hal_adc_ex.c
    PROPERTIES COMPILE_OPTIONS "-Wno-unused-parameter")


# Create SILVER blackpill target
add_library(silver_config INTERFACE)
target_include_directories(silver_config
    INTERFACE
        src/silver
)
target_sources(silver_config
    INTERFACE
        external/stm32_device/Source/Templates/gcc/startup_stm32f401xc.s
)
target_compile_definitions(silver_config
    INTERFACE
        STM32F401xC
)

add_executable(silver_bootloader src/bootloader/main.c)
target_link_libraries(silver_bootloader PRIVATE blackpill_bootloader)
target_link_libraries(silver_bootloader PRIVATE silver_config)
target_sources(silver_bootloader
    PRIVATE
        src/bootloader/stm32f4xx_hal_systick.c
)
target_link_options(silver_bootloader
    PRIVATE
        -T${CMAKE_SOURCE_DIR}/src/silver/STM32F401CCUx_FLASH_BOOTLOADER.ld
)
add_custom_command(TARGET silver_bootloader
    POST_BUILD
        COMMAND arm-none-eabi-size silver_bootloader
)
add_custom_command(TARGET silver_bootloader
    POST_BUILD
        COMMAND arm-none-eabi-objcopy -O ihex silver_bootloader silver_bootloader.hex
        COMMAND arm-none-eabi-objcopy -O binary silver_bootloader silver_bootloader.bin
)

add_executable(silver_firmware src/main.c)
target_link_libraries(silver_firmware PRIVATE blackpill_firmware)
target_link_libraries(silver_firmware PRIVATE silver_config)
target_link_options(silver_firmware
    PRIVATE
        -T${CMAKE_SOURCE_DIR}/src/silver/STM32F401CCUx_FLASH_APPLICATION.ld
)
add_custom_command(TARGET silver_firmware
    POST_BUILD
        COMMAND arm-none-eabi-size silver_firmware
)
add_custom_command(TARGET silver_firmware
    POST_BUILD
        COMMAND arm-none-eabi-objcopy -O ihex silver_firmware silver_firmware.hex
        COMMAND arm-none-eabi-objcopy -O binary silver_firmware silver_firmware.bin
)

set_target_properties(silver_bootloader silver_firmware
    PROPERTIES
        C_STANDARD_REQUIRED ON
        C_EXTENSIONS OFF
)

add_custom_target(silver
        dd if=${CMAKE_BINARY_DIR}/silver_bootloader.bin of=${CMAKE_BINARY_DIR}/silver.bin
    COMMAND
        dd if=${CMAKE_BINARY_DIR}/silver_firmware.bin of=${CMAKE_BINARY_DIR}/silver.bin bs=4K seek=8
    DEPENDS
        silver_bootloader silver_firmware
)

find_program(OPEN_OCD openocd REQUIRED)

add_custom_target(flash
    ${OPEN_OCD}
        -f interface/stlink.cfg
        -f target/stm32f4x.cfg
        -c \"init\; reset halt\; stm32f4x unlock 0\"
        -c \"flash write_image erase ${CMAKE_BINARY_DIR}/silver.bin 0x08000000\"
        -c \"reset\; exit\"
)

add_custom_target(erase
    ${OPEN_OCD}
        -f interface/stlink.cfg
        -f target/stm32f4x.cfg
        -c \"init\; reset halt\; stm32f4x unlock 0\"
        -c \"flash erase_sector 0 0 last\"
        -c \"reset\; exit\"
)




# Create GOLD blackpill target
add_library(gold_config INTERFACE)
target_include_directories(gold_config
    INTERFACE
        src/gold
)
target_sources(gold_config
    INTERFACE
        external/stm32_device/Source/Templates/gcc/startup_stm32f411xe.s
)
target_compile_definitions(gold_config
    INTERFACE
        STM32F411xE
)

add_executable(gold_bootloader src/bootloader/main.c)
target_link_libraries(gold_bootloader PRIVATE blackpill_bootloader)
target_link_libraries(gold_bootloader PRIVATE gold_config)
target_sources(gold_bootloader
    PRIVATE
        src/bootloader/stm32f4xx_hal_systick.c
)
target_link_options(gold_bootloader
    PRIVATE
        -T${CMAKE_SOURCE_DIR}/src/gold/STM32F411CEUx_FLASH_BOOTLOADER.ld
)
add_custom_command(TARGET gold_bootloader
    POST_BUILD
        COMMAND arm-none-eabi-size gold_bootloader
)
add_custom_command(TARGET gold_bootloader
    POST_BUILD
        COMMAND arm-none-eabi-objcopy -O ihex gold_bootloader gold_bootloader.hex
        COMMAND arm-none-eabi-objcopy -O binary gold_bootloader gold_bootloader.bin
)

add_executable(gold_firmware src/main.c)
target_link_libraries(gold_firmware PRIVATE blackpill_firmware)
target_link_libraries(gold_firmware PRIVATE gold_config)
target_link_options(gold_firmware
    PRIVATE
        -T${CMAKE_SOURCE_DIR}/src/gold/STM32F411CEUx_FLASH_APPLICATION.ld
)
add_custom_command(TARGET gold_firmware
    POST_BUILD
        COMMAND arm-none-eabi-size gold_firmware
)
add_custom_command(TARGET gold_firmware
    POST_BUILD
        COMMAND arm-none-eabi-objcopy -O ihex gold_firmware gold_firmware.hex
        COMMAND arm-none-eabi-objcopy -O binary gold_firmware gold_firmware.bin
)

set_target_properties(gold_bootloader gold_firmware
    PROPERTIES
        C_STANDARD_REQUIRED ON
        C_EXTENSIONS OFF
)

add_custom_target(gold
        dd if=${CMAKE_BINARY_DIR}/gold_bootloader.bin of=${CMAKE_BINARY_DIR}/gold.bin
    COMMAND
        dd if=${CMAKE_BINARY_DIR}/gold_firmware.bin of=${CMAKE_BINARY_DIR}/gold.bin bs=4K seek=8
    DEPENDS
        gold_bootloader gold_firmware
)
