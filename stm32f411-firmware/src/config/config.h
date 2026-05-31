/**
 * @file config.h
 * @brief Global hardware configuration and pin definitions for STM32F411
 */

#ifndef CONFIG_H
#define CONFIG_H

#include "stm32f4xx_hal.h"

// ============================================================================
// GPIO Definitions
// ============================================================================
#define LED_PIN             GPIO_PIN_13
#define LED_PORT            GPIOC
#define LED_ON              GPIO_PIN_RESET  // Active low on Black Pill
#define LED_OFF             GPIO_PIN_SET

#define BUZZER_PIN          GPIO_PIN_12
#define BUZZER_PORT         GPIOB
#define BUZZER_ON           GPIO_PIN_SET    // Active high
#define BUZZER_OFF          GPIO_PIN_RESET

#define RS485_DIR_PIN       GPIO_PIN_4
#define RS485_DIR_PORT      GPIOA
#define RS485_TX_ENABLE     GPIO_PIN_SET
#define RS485_RX_ENABLE     GPIO_PIN_RESET

// ============================================================================
// Communication Interfaces
// ============================================================================
extern UART_HandleTypeDef huart1;
extern UART_HandleTypeDef huart2;
#define DEBUG_UART          &huart1
#define MODBUS_UART         &huart2
#define MODBUS_BAUDRATE     9600

extern I2C_HandleTypeDef hi2c1;
#define SENSOR_I2C          &hi2c1
#define INA219_ADDR         (0x40 << 1)     // STM32 HAL requires shifted 8-bit address
#define BMP280_ADDR         (0x76 << 1)     // Default 0x76 shifted

// ============================================================================
// Analog & Motor Control
// ============================================================================
extern ADC_HandleTypeDef hadc1;
#define LM35_ADC            &hadc1

extern TIM_HandleTypeDef htim1;
#define MOTOR_PWM_TIM       &htim1
#define MOTOR_PWM_CHANNEL   TIM_CHANNEL_1

// ============================================================================
// System Constants
// ============================================================================
#define ADC_VREF_MV         3300U           // 3.3V reference in mV
#define ADC_MAX_VAL         4095U           // 12-bit ADC

// ============================================================================
// Sensor Timeouts
// ============================================================================
#define INA219_TIMEOUT_MS   50U
#define BMP280_TIMEOUT_MS   100U

// ============================================================================
// Motor Control Thresholds (Phase 2)
// ============================================================================
#define MOTOR_TEMP_WARN_C10     450U        // 45.0°C warning (°C × 10)
#define MOTOR_TEMP_FAULT_C10    650U        // 65.0°C fault (°C × 10)
#define MOTOR_TEMP_HYSTERESIS   50U         // 5.0°C hysteresis
#define MOTOR_CURRENT_ALARM_MA  1000        // 1.0A alarm
#define MOTOR_CURRENT_FAULT_MA  2000        // 2.0A fault

// ============================================================================
// Auto Mode PWM Thresholds (°C × 10)
// ============================================================================
#define MOTOR_AUTO_TEMP_LOW     350U        // < 35°C: PWM = 0%
#define MOTOR_AUTO_TEMP_MED1    450U        // 35-45°C: PWM = 40%
#define MOTOR_AUTO_TEMP_MED2    550U        // 45-55°C: PWM = 70%
#define MOTOR_AUTO_TEMP_HIGH    650U        // 55-65°C: PWM = 100%

// ============================================================================
// Modbus Slave ID
// ============================================================================
#define MODBUS_SLAVE_ID         1U

// ============================================================================
// Watchdog Configuration
// ============================================================================
#define IWDG_TIMEOUT_MS         2000U       // 2 second watchdog timeout
#define IWDG_FEED_INTERVAL_MS   1000U       // Feed every 1 second

#endif // CONFIG_H
