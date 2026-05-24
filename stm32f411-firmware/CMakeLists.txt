cmake_minimum_required(VERSION 3.22)

#
# This file is generated only once,
# and is not re-generated if converter is called multiple times.
#
# User is free to modify the file as much as necessary
#

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
# User custom sources (drivers, sensors, motor, modbus, etc.)
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
)

# ============================================================================
# User include paths
# ============================================================================
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
    src
    src/config
    src/drivers
    src/sensors
    src/modbus
    src/motor
    src/fault
    src/freertos
)

# ============================================================================
# Linked libraries
# ============================================================================
target_link_libraries(${CMAKE_PROJECT_NAME}
    stm32cubemx
)
