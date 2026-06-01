/**
 * @file motor_control.c
 * @brief Motor control state machine implementation
 *
 * Implements:
 *   - 4-state FSM: STOP, RUNNING, WARNING, FAULT
 *   - Manual mode: User sets PWM directly
 *   - Auto mode: PWM based on LM35 temperature mapping
 *   - Over-temperature protection (alarm 45°C, fault 65°C)
 *   - Over-current protection (alarm 1.0A, fault 2.0A)
 *   - Hysteresis to prevent oscillation
 *   - PWM duty cycle output via TIM1_CH1
 *
 * @version 1.0
 * @date 2026-05-24
 */

#include "motor_control.h"
#include "../config/config.h"
#include "../drivers/gpio_driver.h"
#include "../drivers/uart_debug.h"
#include "../fault/fault_manager.h"
#include "../sensors/lm35_test.h"
#include "../sensors/ina219_test.h"
#include <stdio.h>
#include <string.h>

/* ============================================================================
 * Private Definitions
 * ============================================================================ */

/** @brief Default thresholds (can be overridden via Modbus commands) */
#define MOTOR_TEMP_WARN_DEFAULT     450U    /**< 45.0°C (°C × 10) */
#define MOTOR_TEMP_FAULT_DEFAULT    650U    /**< 65.0°C (°C × 10) */
#define MOTOR_TEMP_HYSTERESIS       50U     /**< 5.0°C hysteresis (°C × 10) */
#define MOTOR_CURRENT_ALARM_MA      1000    /**< 1.0A alarm */
#define MOTOR_CURRENT_FAULT_MA      2000    /**< 2.0A fault */
#define MOTOR_WARNING_PWM_REDUCE    50U     /**< Reduce to 50% in WARNING */

/** @brief Auto mode temperature-to-PWM lookup table */
typedef struct {
    int16_t  s16_temp_c10_max;  /**< Upper temp limit (°C × 10) */
    uint8_t  u8_pwm;            /**< PWM duty (0-100%) */
} motor_auto_entry_t;

static const motor_auto_entry_t motor_auto_table[] = {
    { 350,   0U },     /* < 35.0°C: Motor OFF */
    { 450,  40U },     /* 35.0 - 45.0°C: 40% */
    { 550,  70U },     /* 45.0 - 55.0°C: 70% */
    { 650, 100U },     /* 55.0 - 65.0°C: 100% */
    { 9999, 100U },    /* > 65.0°C: FAULT (handled separately) */
};

#define AUTO_TABLE_SIZE (sizeof(motor_auto_table) / sizeof(motor_auto_table[0]))

/* ============================================================================
 * Private Variables
 * ============================================================================ */

/** @brief Current motor state */
static motor_state_t e_current_state = MOTOR_STATE_STOP;

/** @brief Previous motor state (for transition detection) */
static motor_state_t e_prev_state = MOTOR_STATE_STOP;

/** @brief Current PWM duty cycle (0-100%) */
static uint8_t u8_current_pwm = 0U;

/** @brief Latest sensor data */
static motor_sensor_data_t s_sensor_data;

/** @brief Configurable thresholds */
static uint16_t u16_temp_warn_threshold  = MOTOR_TEMP_WARN_DEFAULT;
static uint16_t u16_temp_fault_threshold = MOTOR_TEMP_FAULT_DEFAULT;
static uint16_t u16_current_alarm_ma     = MOTOR_CURRENT_ALARM_MA;
static uint16_t u16_current_fault_ma     = MOTOR_CURRENT_FAULT_MA;

/** @brief State change timestamp for tracking */
static uint32_t u32_state_enter_ms = 0U;

/** @brief Debug print timestamp (avoid UART flood) */
static uint32_t u32_last_debug_ms = 0U;

/* ============================================================================
 * Private Functions
 * ============================================================================ */

/**
 * @brief State entry actions
 */
static void motor_state_enter(motor_state_t e_new_state, uint32_t u32_now_ms)
{
    e_prev_state = e_current_state;
    e_current_state = e_new_state;
    u32_state_enter_ms = u32_now_ms;

    switch (e_new_state) {
        case MOTOR_STATE_STOP:
            u8_current_pwm = 0U;
            __HAL_TIM_SET_COMPARE(MOTOR_PWM_TIM, MOTOR_PWM_CHANNEL, 0U);
            gpio_set_led(0U);
            gpio_set_buzzer(0U);
            uart_debug_printf("[MOTOR] State -> STOP (PWM=0%%)\r\n");
            break;

        case MOTOR_STATE_RUNNING:
            gpio_set_led(1U);
            gpio_set_buzzer(0U);
            uart_debug_printf("[MOTOR] State -> RUNNING\r\n");
            break;

        case MOTOR_STATE_WARNING:
            gpio_set_led(1U);
            gpio_set_buzzer(1U);
            uart_debug_printf("[MOTOR] State -> WARNING\r\n");
            break;

        case MOTOR_STATE_FAULT:
            u8_current_pwm = 0U;
            __HAL_TIM_SET_COMPARE(MOTOR_PWM_TIM, MOTOR_PWM_CHANNEL, 0U);
            gpio_set_led(0U);
            gpio_set_buzzer(1U);  /* Continuous buzzer */
            uart_debug_printf("[MOTOR] State -> FAULT (PWM=0%%, BUZZER ON)\r\n");
            break;
    }
}

/**
 * @brief State exit actions
 */
static void motor_state_exit(motor_state_t e_old_state)
{
    if (e_old_state == MOTOR_STATE_FAULT) {
        gpio_set_buzzer(0U);  /* Stop continuous buzzer */
    }
}

/**
 * @brief Apply PWM to motor hardware
 */
static void motor_apply_pwm(uint8_t u8_duty)
{
    /* TIM1 period is 99 (0-99 = 0-100%) */
    uint32_t u32_compare = ((uint32_t)u8_duty * 99U) / 100U;
    __HAL_TIM_SET_COMPARE(MOTOR_PWM_TIM, MOTOR_PWM_CHANNEL, u32_compare);
    u8_current_pwm = u8_duty;
}

/**
 * @brief Get PWM setpoint from auto mode temperature lookup
 */
static uint8_t motor_auto_get_pwm(int16_t s16_temp_c10)
{
    for (uint8_t u8_i = 0U; u8_i < AUTO_TABLE_SIZE; u8_i++) {
        if (s16_temp_c10 < motor_auto_table[u8_i].s16_temp_c10_max) {
            return motor_auto_table[u8_i].u8_pwm;
        }
    }
    return 100U;  /* Max for highest temps */
}

/* ============================================================================
 * Public Functions
 * ============================================================================ */

int motor_control_init(void)
{
    /* BUG FIX: Initialize INA219 before first read (calibration needed) */
    uart_debug_printf("[MOTOR] Initializing INA219 sensor...\r\n");
    if (ina219_test_init() != 0) {
        uart_debug_printf("[MOTOR] WARNING: INA219 init failed - I2C issue?\r\n");
        /* Continue anyway - INA219 has default config, current read may be inaccurate */
    }

    /* Start PWM output */
    HAL_TIM_PWM_Start(MOTOR_PWM_TIM, MOTOR_PWM_CHANNEL);

    /* Set initial state */
    e_current_state = MOTOR_STATE_STOP;
    e_prev_state = MOTOR_STATE_STOP;
    u8_current_pwm = 0U;
    __HAL_TIM_SET_COMPARE(MOTOR_PWM_TIM, MOTOR_PWM_CHANNEL, 0U);

    /* Clear sensor data */
    memset(&s_sensor_data, 0, sizeof(s_sensor_data));

    /* Initialize fault manager */
    fault_manager_init();

    /* Set default thresholds */
    u16_temp_warn_threshold  = MOTOR_TEMP_WARN_DEFAULT;
    u16_temp_fault_threshold = MOTOR_TEMP_FAULT_DEFAULT;
    u16_current_alarm_ma     = MOTOR_CURRENT_ALARM_MA;
    u16_current_fault_ma     = MOTOR_CURRENT_FAULT_MA;

    uart_debug_printf("[MOTOR] Motor control initialized (PWM TIM1_CH1, 20kHz)\r\n");
    return 0;
}

int motor_control_update(const motor_command_t *ps_cmd)
{
    if (ps_cmd == NULL) {
        return -1;
    }

    /* --- 1. Read sensors --- */
    int16_t s16_temp_c10 = 0;
    int16_t s16_current_ma = 0;
    uint16_t u16_voltage_mv = 0;
    uint8_t u8_sensor_errors = 0U;

    if (lm35_test_read_temp_c10(&s16_temp_c10) != 0) {
        u8_sensor_errors |= 0x01U;
        if (s_sensor_data.s16_lm35_temp_c10 != 0) {
            s16_temp_c10 = s_sensor_data.s16_lm35_temp_c10;
        }
    }

    if (ina219_test_read_bus_voltage_mv(&u16_voltage_mv) != 0) {
        u8_sensor_errors |= 0x02U;
        u16_voltage_mv = s_sensor_data.u16_voltage_mv;
    }

    if (ina219_test_read_current_ma(&s16_current_ma) != 0) {
        u8_sensor_errors |= 0x04U;
        s16_current_ma = s_sensor_data.s16_current_ma;
    }

    /* Update sensor data structure */
    s_sensor_data.s16_lm35_temp_c10 = s16_temp_c10;
    s_sensor_data.u16_voltage_mv    = u16_voltage_mv;
    s_sensor_data.s16_current_ma    = s16_current_ma;
    s_sensor_data.u8_sensor_errors  = u8_sensor_errors;

    /* --- 2. Update configurable thresholds from command --- */
    if (ps_cmd->u16_current_limit > 0U) {
        u16_current_alarm_ma = ps_cmd->u16_current_limit;
        u16_current_fault_ma = ps_cmd->u16_current_limit * 2U;
    }
    if (ps_cmd->u16_temp_warn > 0U) {
        u16_temp_warn_threshold = ps_cmd->u16_temp_warn;
    }
    if (ps_cmd->u16_temp_fault > 0U) {
        u16_temp_fault_threshold = ps_cmd->u16_temp_fault;
    }

    /* --- 3. Handle reset fault command --- */
    if (ps_cmd->u8_reset_fault == 1U && e_current_state == MOTOR_STATE_FAULT) {
        if (u8_sensor_errors == 0U) {
            /* Only allow reset if sensors are OK */
            motor_state_exit(e_current_state);
            motor_state_enter(MOTOR_STATE_STOP, HAL_GetTick());
            fault_manager_clear_all();
            uart_debug_printf("[MOTOR] Fault reset accepted\r\n");
        } else {
            uart_debug_printf("[MOTOR] Fault reset rejected (sensor errors active)\r\n");
        }
    }

    /* --- 4. State machine --- */
    motor_state_t e_next_state = e_current_state;

    switch (e_current_state) {
        case MOTOR_STATE_STOP:
            if (ps_cmd->u8_enable == 1U && ps_cmd->u8_pwm_setpoint > 0U) {
                /* BUG FIX: Allow start even with sensor errors (with warning)
                 * Only block if BOTH LM35 AND INA219 fail completely */
                if ((u8_sensor_errors & 0x01U) != 0U && (u8_sensor_errors & 0x06U) != 0U) {
                    /* Both LM35 AND INA219 failed - truly unsafe */
                    uart_debug_printf("[MOTOR] BLOCKED: All sensors failed (0x%02X)\r\n",
                                      u8_sensor_errors);
                } else {
                    if (u8_sensor_errors != 0U) {
                        uart_debug_printf("[MOTOR] WARN: Starting with sensor errors (0x%02X)\r\n",
                                          u8_sensor_errors);
                    }
                    e_next_state = MOTOR_STATE_RUNNING;
                }
            }
            break;

        case MOTOR_STATE_RUNNING:
            /* Check over-temperature fault */
            if (s16_temp_c10 > (int16_t)u16_temp_fault_threshold) {
                e_next_state = MOTOR_STATE_FAULT;
                fault_manager_report(FAULT_OVERHEAT);
            }
            /* Check over-current fault */
            else if (s16_current_ma > (int16_t)u16_current_fault_ma) {
                e_next_state = MOTOR_STATE_FAULT;
                fault_manager_report(FAULT_OVERCURRENT);
            }
            /* Check temperature warning */
            else if (s16_temp_c10 > (int16_t)u16_temp_warn_threshold) {
                e_next_state = MOTOR_STATE_WARNING;
                fault_manager_report(FAULT_TEMP_WARNING);
            }
            /* Check disable */
            if (ps_cmd->u8_enable == 0U) {
                e_next_state = MOTOR_STATE_STOP;
            }
            break;

        case MOTOR_STATE_WARNING:
            /* Check over-temperature fault */
            if (s16_temp_c10 > (int16_t)u16_temp_fault_threshold) {
                e_next_state = MOTOR_STATE_FAULT;
                fault_manager_report(FAULT_OVERHEAT);
            }
            /* Check over-current fault */
            else if (s16_current_ma > (int16_t)u16_current_fault_ma) {
                e_next_state = MOTOR_STATE_FAULT;
                fault_manager_report(FAULT_OVERCURRENT);
            }
            /* Hysteresis: exit WARNING when temp drops below threshold - hysteresis */
            else if (s16_temp_c10 < ((int16_t)u16_temp_warn_threshold - (int16_t)MOTOR_TEMP_HYSTERESIS)) {
                e_next_state = MOTOR_STATE_RUNNING;
                uart_debug_printf("[MOTOR] WARNING -> RUNNING (temp < %.1f°C)\r\n",
                                  (float)(u16_temp_warn_threshold - MOTOR_TEMP_HYSTERESIS) / 10.0f);
            }
            /* Check disable */
            if (ps_cmd->u8_enable == 0U) {
                e_next_state = MOTOR_STATE_STOP;
            }
            break;

        case MOTOR_STATE_FAULT:
            /* Stay in FAULT until explicit reset (handled in step 3) */
            break;
    }

    /* Execute state transition */
    if (e_next_state != e_current_state) {
        motor_state_exit(e_current_state);
        motor_state_enter(e_next_state, HAL_GetTick());
    }

    /* --- 5. Update PWM based on current state and mode --- */
    if (e_current_state == MOTOR_STATE_STOP || e_current_state == MOTOR_STATE_FAULT) {
        motor_apply_pwm(0U);
    } else if (e_current_state == MOTOR_STATE_WARNING) {
        /* WARNING: Apply 50% of user's requested PWM (not the stored value) */
        if (ps_cmd->e_mode == MOTOR_MODE_MANUAL) {
            motor_apply_pwm((ps_cmd->u8_pwm_setpoint * MOTOR_WARNING_PWM_REDUCE) / 100U);
        } else {
            motor_apply_pwm((motor_auto_get_pwm(s16_temp_c10) * MOTOR_WARNING_PWM_REDUCE) / 100U);
        }
    } else if (e_current_state == MOTOR_STATE_RUNNING) {
        if (ps_cmd->e_mode == MOTOR_MODE_MANUAL) {
            motor_apply_pwm(ps_cmd->u8_pwm_setpoint);
        } else {
            uint8_t u8_auto_pwm = motor_auto_get_pwm(s16_temp_c10);
            motor_apply_pwm(u8_auto_pwm);
        }
    }

    /* --- 6. Debug output (every 1 second to avoid UART flood) --- */
    uint32_t u32_now_ms = HAL_GetTick();
    if ((u32_now_ms - u32_last_debug_ms) >= 1000U) {
        u32_last_debug_ms = u32_now_ms;
        uart_debug_printf("[MOTOR] S=%d PWM=%d%% T=%.1fC I=%dmA V=%dmV ALM=0x%04X ERR=0x%02X\r\n",
                          (int)e_current_state,
                          u8_current_pwm,
                          (float)s16_temp_c10 / 10.0f,
                          s16_current_ma,
                          u16_voltage_mv,
                          fault_manager_get_alarm_flags(),
                          u8_sensor_errors);
    }

    return 0;
}

motor_state_t motor_get_state(void)
{
    return e_current_state;
}

uint8_t motor_get_pwm(void)
{
    return u8_current_pwm;
}

uint16_t motor_get_alarm_flags(void)
{
    return fault_manager_get_alarm_flags();
}

uint8_t motor_get_fault_code(void)
{
    return fault_manager_get_fault_code();
}

const motor_sensor_data_t *motor_get_sensor_data(void)
{
    return &s_sensor_data;
}
