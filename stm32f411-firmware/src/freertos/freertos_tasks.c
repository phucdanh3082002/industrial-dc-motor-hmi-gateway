/**
 * @file freertos_tasks.c
 * @brief FreeRTOS task definitions for STM32F411 Motor Control
 *
 * Tasks:
 *   - sensor_task (Pri=3, 50ms):  Read LM35 + INA219, send via Queue
 *   - motor_task  (Pri=4, 100ms): FSM + PWM control
 *   - uart_task   (Pri=2, event):  Parse UART commands
 *   - watchdog_task (Pri=5, 1s):  Feed IWDG + health monitor
 */

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "event_groups.h"
#include "semphr.h"

#include "config/config.h"
#include "drivers/gpio_driver.h"
#include "drivers/uart_debug.h"
#include "drivers/i2c_driver.h"
#include "drivers/adc_driver.h"
#include "sensors/lm35_test.h"
#include "sensors/ina219_test.h"
#include "motor/motor_control.h"
#include "fault/fault_manager.h"

/* IWDG handle is defined in main.c */
extern IWDG_HandleTypeDef hiwdg;

#include <stdio.h>
#include <string.h>

/* ============================================================================
 * Type Definitions
 * ============================================================================ */

/** @brief Sensor data structure for queue transmission */
typedef struct {
    int16_t  s16_temp_c10;     /**< LM35 temperature (°C × 10) */
    uint16_t u16_voltage_mv;   /**< INA219 bus voltage (mV) */
    int16_t  s16_current_ma;   /**< INA219 current (mA) */
    uint8_t  u8_sensor_errors; /**< Bitmask: bit0=LM35, bit1=INA219V, bit2=INA219I */
    uint32_t u32_timestamp;    /**< xTaskGetTickCount() when read */
} sensor_msg_t;

/** @brief Command structure from UART to motor task */
typedef struct {
    uint8_t u8_enable;         /**< 0=stop, 1=run */
    uint8_t u8_pwm;            /**< PWM duty (0-100%) */
    uint8_t u8_mode;           /**< 0=manual, 1=auto */
    uint8_t u8_reset_fault;    /**< 1=reset from fault */
} cmd_msg_t;

/* ============================================================================
 * Heartbeat Event Bits
 * ============================================================================ */

#define BIT_SENSOR_ALIVE    (1U << 0U)
#define BIT_MOTOR_ALIVE     (1U << 1U)
#define BIT_UART_ALIVE      (1U << 2U)

/* ============================================================================
 * Static Variables (all tasks share via these)
 * ============================================================================ */

/** @brief Queue: sensor_task → motor_task */
static QueueHandle_t s_sensor_queue = NULL;

/** @brief Queue: uart_task → motor_task */
static QueueHandle_t s_cmd_queue = NULL;

/** @brief Event group for task heartbeat monitoring */
static EventGroupHandle_t s_heartbeat_group = NULL;

/** @brief Semaphore for UART debug print (thread-safe) */
static SemaphoreHandle_t s_uart_mutex = NULL;

/** @brief Current motor command (updated by uart_task) */
static motor_command_t s_motor_cmd;

/** @brief Latest sensor data (updated by sensor_task) */
static sensor_msg_t s_latest_sensor;

/** @brief Watchdog task alive flag */
static volatile uint8_t s_u8_watchdog_feed = 0U;

/* ============================================================================
 * Task: SENSOR_TASK
 * Priority: 3 | Period: 50ms
 * Reads LM35 (ADC) + INA219 (I2C), sends data via queue
 * ============================================================================ */

static void sensor_task(void *pvParameters)
{
    (void)pvParameters;
    sensor_msg_t msg;

    uart_debug_printf("[TASK] sensor_task started (Pri=3, 50ms)\r\n");

    for (;;)
    {
        msg.u8_sensor_errors = 0U;

        /* Read LM35 temperature */
        if (lm35_test_read_temp_c10(&msg.s16_temp_c10) != 0) {
            msg.u8_sensor_errors |= 0x01U;
            msg.s16_temp_c10 = s_latest_sensor.s16_temp_c10;
        }

        /* Read INA219 bus voltage */
        if (ina219_test_read_bus_voltage_mv(&msg.u16_voltage_mv) != 0) {
            msg.u8_sensor_errors |= 0x02U;
            msg.u16_voltage_mv = s_latest_sensor.u16_voltage_mv;
        }

        /* Read INA219 current */
        if (ina219_test_read_current_ma(&msg.s16_current_ma) != 0) {
            msg.u8_sensor_errors |= 0x04U;
            msg.s16_current_ma = s_latest_sensor.s16_current_ma;
        }

        msg.u32_timestamp = xTaskGetTickCount();

        /* Update local cache for fallback */
        if (msg.u8_sensor_errors == 0U) {
            s_latest_sensor = msg;
        }

        /* Send to motor_task (non-blocking, drop if full) */
        xQueueOverwrite(s_sensor_queue, &msg);

        /* Heartbeat */
        xEventGroupSetBits(s_heartbeat_group, BIT_SENSOR_ALIVE);

        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

/* ============================================================================
 * Task: MOTOR_TASK
 * Priority: 4 (highest control) | Period: 100ms
 * Receives sensor data + commands, runs FSM, applies PWM
 * ============================================================================ */

static void motor_task(void *pvParameters)
{
    (void)pvParameters;
    sensor_msg_t sensor;
    cmd_msg_t cmd;
    uint32_t u32_last_debug = 0U;

    uart_debug_printf("[TASK] motor_task started (Pri=4, 100ms)\r\n");

    for (;;)
    {
        /* Non-blocking: peek latest sensor data */
        xQueuePeek(s_sensor_queue, &sensor, 0);

        /* Non-blocking: receive latest command */
        if (xQueueReceive(s_cmd_queue, &cmd, 0) == pdTRUE) {
            s_motor_cmd.u8_enable      = cmd.u8_enable;
            s_motor_cmd.u8_pwm_setpoint = cmd.u8_pwm;
            s_motor_cmd.e_mode         = (cmd.u8_mode == 1U) ? MOTOR_MODE_AUTO : MOTOR_MODE_MANUAL;
            s_motor_cmd.u8_reset_fault = cmd.u8_reset_fault;
        }

        /* Run motor control update (FSM + PWM) */
        motor_control_update(&s_motor_cmd);

        /* Heartbeat */
        xEventGroupSetBits(s_heartbeat_group, BIT_MOTOR_ALIVE);

        /* Debug print every 1 second */
        uint32_t u32_now = xTaskGetTickCount();
        if ((u32_now - u32_last_debug) >= pdMS_TO_TICKS(1000U)) {
            u32_last_debug = u32_now;
            uart_debug_printf("[MOTOR] S=%d PWM=%d%% T=%.1fC I=%dmA V=%dmV ALM=0x%04X\r\n",
                              (int)motor_get_state(),
                              motor_get_pwm(),
                              (float)sensor.s16_temp_c10 / 10.0f,
                              sensor.s16_current_ma,
                              sensor.u16_voltage_mv,
                              motor_get_alarm_flags());
        }

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

/* ============================================================================
 * Task: UART_TASK
 * Priority: 2 | Event-driven (blocking on UART receive)
 * Parses single-byte commands from USART1 (debug UART)
 * ============================================================================ */

static void uart_task(void *pvParameters)
{
    (void)pvParameters;
    uint8_t u8_rx_byte = 0U;
    cmd_msg_t cmd;

    uart_debug_printf("[TASK] uart_task started (Pri=2)\r\n");
    uart_debug_printf("[INIT] Commands: S=start X=stop 0-9=PWM A=auto M=manual R=reset T=test\r\n");

    for (;;)
    {
        /* Blocking receive with timeout - waits for byte from UART1 */
        if (HAL_UART_Receive(&huart1, &u8_rx_byte, 1U, pdMS_TO_TICKS(100)) == HAL_OK)
        {
            /* Start with current command state */
            cmd.u8_enable       = s_motor_cmd.u8_enable;
            cmd.u8_pwm          = s_motor_cmd.u8_pwm_setpoint;
            cmd.u8_mode         = (s_motor_cmd.e_mode == MOTOR_MODE_AUTO) ? 1U : 0U;
            cmd.u8_reset_fault  = 0U;

            switch (u8_rx_byte)
            {
                case 'S': case 's':
                    cmd.u8_enable = 1U;
                    cmd.u8_pwm    = 50U;
                    uart_debug_printf("[CMD] Motor START (50%%)\r\n");
                    break;

                case 'X': case 'x':
                    cmd.u8_enable = 0U;
                    cmd.u8_pwm    = 0U;
                    uart_debug_printf("[CMD] Motor STOP\r\n");
                    break;

                case 'R': case 'r':
                    cmd.u8_reset_fault = 1U;
                    uart_debug_printf("[CMD] Reset fault\r\n");
                    break;

                case '0':
                    cmd.u8_enable = 1U;
                    cmd.u8_pwm    = 100U;
                    uart_debug_printf("[CMD] PWM = 100%%\r\n");
                    break;

                case '1': case '2': case '3': case '4': case '5':
                case '6': case '7': case '8': case '9':
                    cmd.u8_enable = 1U;
                    cmd.u8_pwm    = (u8_rx_byte - '0') * 10U;
                    uart_debug_printf("[CMD] PWM = %d%%\r\n", cmd.u8_pwm);
                    break;

                case 'A': case 'a':
                    cmd.u8_enable = 1U;
                    cmd.u8_mode   = 1U;
                    uart_debug_printf("[CMD] Mode -> AUTO\r\n");
                    break;

                case 'M': case 'm':
                    cmd.u8_mode = 0U;
                    uart_debug_printf("[CMD] Mode -> MANUAL\r\n");
                    break;

                case 'T': case 't':
                    /* Direct PWM test bypass FSM */
                    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, 99U);
                    uart_debug_printf("[DIAG] Direct PWM 100%% (bypass FSM)\r\n");
                    return;  /* Exit task after test */

                case 'Y': case 'y':
                    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, 0U);
                    uart_debug_printf("[DIAG] Direct PWM 0%%\r\n");
                    break;

                default:
                    uart_debug_printf("[CMD] Unknown: '%c' (0x%02X)\r\n",
                                      (char)u8_rx_byte, u8_rx_byte);
                    break;
            }

            /* Send command to motor_task */
            xQueueOverwrite(s_cmd_queue, &cmd);
        }

        /* Heartbeat */
        xEventGroupSetBits(s_heartbeat_group, BIT_UART_ALIVE);
    }
}

/* ============================================================================
 * Task: WATCHDOG_TASK
 * Priority: 5 (highest) | Period: 1s
 * Feeds IWDG + monitors health of all tasks
 * ============================================================================ */

static void watchdog_task(void *pvParameters)
{
    (void)pvParameters;

    uart_debug_printf("[TASK] watchdog_task started (Pri=5, 1s)\r\n");

    for (;;)
    {
        /* Wait for all task heartbeats (with 3s timeout) */
        EventBits_t bits = xEventGroupWaitBits(
            s_heartbeat_group,
            BIT_SENSOR_ALIVE | BIT_MOTOR_ALIVE | BIT_UART_ALIVE,
            pdTRUE,   /* Clear bits on exit */
            pdFALSE,  /* OR (any bit) */
            pdMS_TO_TICKS(3000U)
        );

        /* Check each task */
        if ((bits & BIT_SENSOR_ALIVE) == 0U) {
            uart_debug_printf("[WDG] WARNING: sensor_task heartbeat missed!\r\n");
            fault_manager_report(FAULT_SENSOR_LM35);
        }

        if ((bits & BIT_MOTOR_ALIVE) == 0U) {
            uart_debug_printf("[WDG] CRITICAL: motor_task frozen! Resetting...\r\n");
            /* Motor task frozen = dangerous → let watchdog reset */
            for (;;) { }  /* Never feed IWDG → hardware reset */
        }

        if ((bits & BIT_UART_ALIVE) == 0U) {
            uart_debug_printf("[WDG] WARNING: uart_task heartbeat missed!\r\n");
        }

        /* Feed IWDG if all tasks alive */
        if ((bits & (BIT_SENSOR_ALIVE | BIT_MOTOR_ALIVE)) != 0U) {
            HAL_IWDG_Refresh(&hiwdg);
            s_u8_watchdog_feed = 1U;
        }

        /* Auto-recovery: clear stale faults every 30s */
        fault_manager_update(xTaskGetTickCount());

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

/* ============================================================================
 * Public Initialization Functions
 * ============================================================================ */

/**
 * @brief Create all FreeRTOS objects (queues, event groups, mutexes)
 */
void freertos_objects_init(void)
{
    s_sensor_queue    = xQueueCreate(1, sizeof(sensor_msg_t));
    s_cmd_queue       = xQueueCreate(1, sizeof(cmd_msg_t));
    s_heartbeat_group = xEventGroupCreate();
    s_uart_mutex      = xSemaphoreCreateMutex();

    configASSERT(s_sensor_queue != NULL);
    configASSERT(s_cmd_queue != NULL);
    configASSERT(s_heartbeat_group != NULL);
    configASSERT(s_uart_mutex != NULL);

    /* Initialize default motor command */
    s_motor_cmd.u8_enable         = 0U;
    s_motor_cmd.u8_pwm_setpoint   = 0U;
    s_motor_cmd.e_mode            = MOTOR_MODE_MANUAL;
    s_motor_cmd.u8_reset_fault    = 0U;
    s_motor_cmd.u16_current_limit = 1000U;
    s_motor_cmd.u16_temp_warn     = 450U;
    s_motor_cmd.u16_temp_fault    = 650U;

    /* Initialize sensor cache */
    memset(&s_latest_sensor, 0, sizeof(s_latest_sensor));

    uart_debug_printf("[INIT] FreeRTOS objects created\r\n");
}

/**
 * @brief Create and start all FreeRTOS tasks
 */
void freertos_tasks_init(void)
{
    xTaskCreate(sensor_task,   "SENSOR",   512, NULL, 3, NULL);
    xTaskCreate(motor_task,    "MOTOR",    512, NULL, 4, NULL);
    xTaskCreate(uart_task,     "UART",     256, NULL, 2, NULL);
    xTaskCreate(watchdog_task, "WATCHDOG", 256, NULL, 5, NULL);

    uart_debug_printf("[INIT] FreeRTOS tasks created\r\n");
    uart_debug_printf("[INIT] Starting scheduler...\r\n");

    vTaskStartScheduler();

    /* Should never reach here */
    uart_debug_printf("[INIT] ERROR: Scheduler exited!\r\n");
    for (;;) { }
}
