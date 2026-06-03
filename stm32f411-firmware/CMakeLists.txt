cmake_minimum_required(VERSION 3.22)

# Setup compiler settings
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)

# Define the build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug")
endif()

# Set the project name
set(CMAKE_PROJECT_NAME stm32f411-firmware)

# Include toolchain file
include("cmake/gcc-arm-none-eabi.cmake")

# Enable compile command to ease indexing with e.g. clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

# Core project settings
project(${CMAKE_PROJECT_NAME})

# Enable CMake support for ASM and C languages
enable_language(C ASM)

message("Build type: " ${CMAKE_BUILD_TYPE})

# Create an executable object type
add_executable(${CMAKE_PROJECT_NAME})

# ============================================================================
# STM32CubeMX generated sources (HAL drivers, peripherals, startup)
# ============================================================================
add_subdirectory(cmake/stm32cubemx)

# ============================================================================
# FreeRTOS kernel sources
# ============================================================================
set(FREERTOS_KERNEL_DIR ${CMAKE_SOURCE_DIR}/Middlewares/FreeRTOS/Kernel)

target_sources(${CMAKE_PROJECT_NAME} PRIVATE
    # FreeRTOS kernel
    ${FREERTOS_KERNEL_DIR}/tasks.c
    ${FREERTOS_KERNEL_DIR}/queue.c
    ${FREERTOS_KERNEL_DIR}/list.c
    ${FREERTOS_KERNEL_DIR}/timers.c
    ${FREERTOS_KERNEL_DIR}/event_groups.c
    ${FREERTOS_KERNEL_DIR}/stream_buffer.c

    # FreeRTOS portable (Cortex-M4F, heap_4)
    ${FREERTOS_KERNEL_DIR}/portable/GCC/ARM_CM4F/port.c
    ${FREERTOS_KERNEL_DIR}/portable/MemMang/heap_4.c
)

# ============================================================================
# User custom sources (drivers, sensors, motor, freertos tasks, etc.)
# ============================================================================
target_sources(${CMAKE_PROJECT_NAME} PRIVATE
    # Driver layer
    src/drivers/adc_driver.c
    src/drivers/gpio_driver.c
    src/drivers/i2c_driver.c
    src/drivers/uart_debug.c

    # Sensor drivers
    src/sensors/lm35_test.c
    src/sensors/ina219_test.c
    src/sensors/bmp280_test.c

    # Motor control & fault management (Phase 2)
    src/motor/motor_control.c
    src/fault/fault_manager.c

    # FreeRTOS tasks
    src/freertos/freertos_tasks.c
)

# ============================================================================
# Include paths
# ============================================================================
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
    # User code
    src
    src/config
    src/drivers
    src/sensors
    src/modbus
    src/motor
    src/fault
    src/freertos

    # FreeRTOS
    ${FREERTOS_KERNEL_DIR}/include
    ${FREERTOS_KERNEL_DIR}/portable/GCC/ARM_CM4F

    # Inc (for FreeRTOSConfig.h)
    Inc
)

# ============================================================================
# Linked libraries
# ============================================================================
target_link_libraries(${CMAKE_PROJECT_NAME}
    stm32cubemx
)
