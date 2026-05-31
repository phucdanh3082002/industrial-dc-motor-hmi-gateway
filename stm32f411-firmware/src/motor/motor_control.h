/**
 * @file motor_control.h
 * @brief Motor control state machine and PWM management
 *
 * Implements the motor FSM with support for manual and automatic modes,
 * over-current and over-temperature protection, and fault recovery.
 *
 * States: STOP -> RUNNING -> WARNING -> FAULT
 * Modes:  MANUAL (user sets PWM), AUTO (PWM based on LM35 temperature)
 *
 * @version 1.0
 * @date 2026-05-24
 */

#ifndef MOTOR_CONTROL_H
#define MOTOR_CONTROL_H

#include <stdint.h>

/* ============================================================================
 * Type Definitions
 * ============================================================================ */

/**
 * @brief Motor operating states
 */
typedef enum {
    MOTOR_STATE_STOP    = 0,  /**< Motor stopped, PWM = 0% */
    MOTOR_STATE_RUNNING = 1,  /**< Motor running normally */
    MOTOR_STATE_WARNING = 2,  /**< Soft limit applied, reduced PWM */
    MOTOR_STATE_FAULT   = 3   /**< Critical fault, immediate shutdown */
} motor_state_t;

/**
 * @brief Motor control modes
 */
typedef enum {
    MOTOR_MODE_MANUAL = 0,    /**< User sets PWM directly */
    MOTOR_MODE_AUTO   = 1     /**< Automatic PWM based on LM35 temperature */
} motor_mode_t;

/**
 * @brief Motor command structure (from Modbus master or local)
 */
typedef struct {
    uint8_t  u8_enable;          /**< 0 = stop, 1 = run */
    uint8_t  u8_pwm_setpoint;    /**< Desired PWM duty (0-100%) */
    motor_mode_t e_mode;         /**< MANUAL or AUTO */
    uint8_t  u8_reset_fault;     /**< 1 = reset from FAULT state */
    uint16_t u16_current_limit;  /**< Over-current threshold (mA) */
    uint16_t u16_temp_warn;      /**< Temperature warning threshold (°C × 10) */
    uint16_t u16_temp_fault;     /**< Temperature fault threshold (°C × 10) */
} motor_command_t;

/**
 * @brief Latest sensor readings (updated by sensor loop)
 */
typedef struct {
    int16_t  s16_lm35_temp_c10;  /**< LM35 temperature (°C × 10) */
    uint16_t u16_voltage_mv;     /**< INA219 bus voltage (mV) */
    int16_t  s16_current_ma;     /**< INA219 current (mA) */
    uint8_t  u8_sensor_errors;   /**< Bitmask: bit0=LM35, bit1=INA219 */
} motor_sensor_data_t;

/* ============================================================================
 * Public API
 * ============================================================================ */

/**
 * @brief Initialize motor control module
 * @return 0 on success
 */
int motor_control_init(void);

/**
 * @brief Run one iteration of the motor state machine
 *
 * Reads sensors, checks protection thresholds, and updates motor state/PWM.
 * Must be called periodically from the main loop (~100ms interval).
 *
 * @param[in] ps_cmd Pointer to motor command structure
 * @return 0 on success, -1 on error
 */
int motor_control_update(const motor_command_t *ps_cmd);

/**
 * @brief Get current motor state
 * @return Current motor state (STOP, RUNNING, WARNING, FAULT)
 */
motor_state_t motor_get_state(void);

/**
 * @brief Get current motor PWM duty cycle
 * @return PWM duty (0-100%)
 */
uint8_t motor_get_pwm(void);

/**
 * @brief Get current alarm flags bitmask
 * @return 16-bit alarm flags
 */
uint16_t motor_get_alarm_flags(void);

/**
 * @brief Get current fault code
 * @return Fault code (0 = no fault)
 */
uint8_t motor_get_fault_code(void);

/**
 * @brief Get sensor data structure
 * @return Pointer to latest sensor readings
 */
const motor_sensor_data_t *motor_get_sensor_data(void);

#endif /* MOTOR_CONTROL_H */
