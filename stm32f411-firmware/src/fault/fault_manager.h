/**
 * @file fault_manager.h
 * @brief Fault detection, logging, and recovery
 *
 * Centralized fault management with escalation rules:
 *   - Occurrence 1: Log info, continue
 *   - Occurrence 2: Log warning, reduce PWM 50%
 *   - Occurrence 3+: Log critical, transition to FAULT state
 *   - Recovery: Auto clear counter after 30s of stability
 *
 * @version 1.0
 * @date 2026-05-24
 */

#ifndef FAULT_MANAGER_H
#define FAULT_MANAGER_H

#include <stdint.h>

/* ============================================================================
 * Fault Codes
 * ============================================================================ */

typedef enum {
    FAULT_NONE                  = 0,  /**< No fault */
    FAULT_OVERCURRENT           = 1,  /**< Motor over-current */
    FAULT_OVERHEAT              = 2,  /**< Motor over-temperature */
    FAULT_TEMP_WARNING          = 3,  /**< Temperature in warning range */
    FAULT_SENSOR_LM35           = 4,  /**< LM35 sensor error */
    FAULT_SENSOR_INA219         = 5,  /**< INA219 sensor error */
    FAULT_SENSOR_BMP280         = 6,  /**< BMP280 sensor error */
    FAULT_WATCHDOG_RESET        = 7,  /**< Watchdog triggered reset */
    FAULT_MODBUS_CRC            = 8,  /**< Modbus CRC error */
    FAULT_CODE_MAX              = 9   /**< Sentinel */
} fault_code_t;

/* ============================================================================
 * Alarm Flags (Bitmask for Modbus register 40009)
 * ============================================================================ */

#define ALARM_FLAG_OVERCURRENT      (1U << 0U)
#define ALARM_FLAG_TEMP_WARNING     (1U << 1U)
#define ALARM_FLAG_OVERHEAT         (1U << 2U)
#define ALARM_FLAG_SENSOR_INA219    (1U << 3U)
#define ALARM_FLAG_SENSOR_BMP280    (1U << 4U)
#define ALARM_FLAG_RS485_TIMEOUT    (1U << 5U)
#define ALARM_FLAG_MQTT_DISCONN     (1U << 6U)
#define ALARM_FLAG_WATCHDOG_RESET   (1U << 7U)

/* ============================================================================
 * Escalation Thresholds
 * ============================================================================ */

#define FAULT_ESCALATION_COUNT_CRITICAL   3U   /**< Trigger FAULT state */
#define FAULT_ESCALATION_COUNT_WARNING    2U   /**< Reduce PWM 50% */
#define FAULT_RECOVERY_TIMEOUT_MS     30000U   /**< Clear counter after 30s */

/* ============================================================================
 * Public API
 * ============================================================================ */

/**
 * @brief Initialize fault manager
 */
void fault_manager_init(void);

/**
 * @brief Report a fault occurrence
 *
 * Increments fault counter. Escalates based on count:
 *   - Count 1: Returns 0 (info)
 *   - Count 2: Returns 1 (warning, reduce PWM)
 *   - Count 3+: Returns 2 (critical, enter FAULT)
 *
 * @param[in] e_code Fault code
 * @return 0=info, 1=warning, 2=critical
 */
int fault_manager_report(fault_code_t e_code);

/**
 * @brief Clear a specific fault
 * @param[in] e_code Fault code to clear
 */
void fault_manager_clear(fault_code_t e_code);

/**
 * @brief Clear all active faults
 */
void fault_manager_clear_all(void);

/**
 * @brief Check if any critical fault is active
 * @return 1 if critical fault active, 0 otherwise
 */
int fault_manager_has_critical(void);

/**
 * @brief Get current alarm flags bitmask
 * @return 16-bit alarm flags
 */
uint16_t fault_manager_get_alarm_flags(void);

/**
 * @brief Get fault code for Modbus register 40010
 * @return Most severe active fault code
 */
uint8_t fault_manager_get_fault_code(void);

/**
 * @brief Update fault recovery timers
 *
 * Call periodically to auto-clear stale fault counters.
 * @param[in] u32_now_ms Current timestamp in milliseconds
 */
void fault_manager_update(uint32_t u32_now_ms);

/**
 * @brief Get sensor error count for Modbus register 40012
 * @return Total sensor error count
 */
uint16_t fault_manager_get_sensor_error_count(void);

#endif /* FAULT_MANAGER_H */
