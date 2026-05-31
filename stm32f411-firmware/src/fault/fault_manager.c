/**
 * @file fault_manager.c
 * @brief Fault manager implementation with escalation
 */

#include "fault_manager.h"
#include "../drivers/gpio_driver.h"
#include "../drivers/uart_debug.h"
#include <string.h>

/* ============================================================================
 * Private Definitions
 * ============================================================================ */

#define FAULT_MAX_CODES  FAULT_CODE_MAX

/* ============================================================================
 * Private Variables
 * ============================================================================ */

/** @brief Fault occurrence counter per fault code */
static uint8_t au8_fault_count[FAULT_MAX_CODES];

/** @brief Timestamp of last occurrence per fault code */
static uint32_t au32_fault_timestamp[FAULT_MAX_CODES];

/** @brief Active alarm flags (bitmask) */
static uint16_t u16_alarm_flags;

/** @brief Total sensor error counter */
static uint16_t u16_sensor_error_count;

/* ============================================================================
 * Private Functions
 * ============================================================================ */

/**
 * @brief Get array index for fault code
 */
static int fault_code_to_index(fault_code_t e_code)
{
    if ((int)e_code < 0 || (int)e_code >= FAULT_MAX_CODES) {
        return -1;
    }
    return (int)e_code;
}

/**
 * @brief Map fault code to alarm flag bit
 */
static uint16_t fault_code_to_flag(fault_code_t e_code)
{
    switch (e_code) {
        case FAULT_OVERCURRENT:    return ALARM_FLAG_OVERCURRENT;
        case FAULT_TEMP_WARNING:   return ALARM_FLAG_TEMP_WARNING;
        case FAULT_OVERHEAT:       return ALARM_FLAG_OVERHEAT;
        case FAULT_SENSOR_LM35:    return ALARM_FLAG_SENSOR_INA219;  /* shares bit */
        case FAULT_SENSOR_INA219:  return ALARM_FLAG_SENSOR_INA219;
        case FAULT_SENSOR_BMP280:  return ALARM_FLAG_SENSOR_BMP280;
        case FAULT_WATCHDOG_RESET: return ALARM_FLAG_WATCHDOG_RESET;
        case FAULT_MODBUS_CRC:     return ALARM_FLAG_RS485_TIMEOUT;
        default:                   return 0U;
    }
}

/* ============================================================================
 * Public Functions
 * ============================================================================ */

void fault_manager_init(void)
{
    memset(au8_fault_count, 0, sizeof(au8_fault_count));
    memset(au32_fault_timestamp, 0, sizeof(au32_fault_timestamp));
    u16_alarm_flags = 0U;
    u16_sensor_error_count = 0U;

    uart_debug_printf("[FAULT] Fault manager initialized\r\n");
}

int fault_manager_report(fault_code_t e_code)
{
    int s32_idx = fault_code_to_index(e_code);
    if (s32_idx < 0) {
        return -1;
    }

    /* Increment counter */
    au8_fault_count[s32_idx]++;

    /* Set alarm flag */
    u16_alarm_flags |= fault_code_to_flag(e_code);

    /* Track sensor errors separately */
    if (e_code == FAULT_SENSOR_LM35 || e_code == FAULT_SENSOR_INA219 ||
        e_code == FAULT_SENSOR_BMP280) {
        u16_sensor_error_count++;
    }

    uart_debug_printf("[FAULT] Code=%d, Count=%d\r\n",
                      (int)e_code, au8_fault_count[s32_idx]);

    /* Escalation logic */
    if (au8_fault_count[s32_idx] >= FAULT_ESCALATION_COUNT_CRITICAL) {
        uart_debug_printf("[FAULT] CRITICAL! Entering FAULT state\r\n");
        return 2;  /* Critical */
    } else if (au8_fault_count[s32_idx] >= FAULT_ESCALATION_COUNT_WARNING) {
        uart_debug_printf("[FAULT] WARNING! Reduce PWM 50%%\r\n");
        return 1;  /* Warning */
    }

    return 0;  /* Info */
}

void fault_manager_clear(fault_code_t e_code)
{
    int s32_idx = fault_code_to_index(e_code);
    if (s32_idx < 0) {
        return;
    }

    au8_fault_count[s32_idx] = 0U;
    au32_fault_timestamp[s32_idx] = 0U;
    u16_alarm_flags &= ~fault_code_to_flag(e_code);

    uart_debug_printf("[FAULT] Cleared fault code=%d\r\n", (int)e_code);
}

void fault_manager_clear_all(void)
{
    memset(au8_fault_count, 0, sizeof(au8_fault_count));
    memset(au32_fault_timestamp, 0, sizeof(au32_fault_timestamp));
    u16_alarm_flags = 0U;

    uart_debug_printf("[FAULT] All faults cleared\r\n");
}

int fault_manager_has_critical(void)
{
    /* Check if any fault has reached escalation threshold */
    for (int i = 0; i < FAULT_MAX_CODES; i++) {
        if (au8_fault_count[i] >= FAULT_ESCALATION_COUNT_CRITICAL) {
            return 1;
        }
    }
    return 0;
}

uint16_t fault_manager_get_alarm_flags(void)
{
    return u16_alarm_flags;
}

uint8_t fault_manager_get_fault_code(void)
{
    /* Return most severe active fault code */
    for (int i = FAULT_MAX_CODES - 1; i >= 0; i--) {
        if (au8_fault_count[i] >= FAULT_ESCALATION_COUNT_CRITICAL) {
            return (uint8_t)i;
        }
    }
    /* Return highest warning if no critical */
    for (int i = FAULT_MAX_CODES - 1; i >= 0; i--) {
        if (au8_fault_count[i] >= FAULT_ESCALATION_COUNT_WARNING) {
            return (uint8_t)i;
        }
    }
    return 0U;
}

void fault_manager_update(uint32_t u32_now_ms)
{
    /* Auto-clear fault counters after recovery timeout */
    for (int i = 0; i < FAULT_MAX_CODES; i++) {
        if (au8_fault_count[i] > 0U && au32_fault_timestamp[i] > 0U) {
            uint32_t u32_elapsed = u32_now_ms - au32_fault_timestamp[i];
            if (u32_elapsed >= FAULT_RECOVERY_TIMEOUT_MS) {
                au8_fault_count[i] = 0U;
                au32_fault_timestamp[i] = 0U;
                u16_alarm_flags &= ~fault_code_to_flag((fault_code_t)i);
                uart_debug_printf("[FAULT] Auto-recovery: fault %d cleared after %lu ms\r\n",
                                  i, (unsigned long)u32_elapsed);
            }
        }
    }
}

uint16_t fault_manager_get_sensor_error_count(void)
{
    return u16_sensor_error_count;
}
