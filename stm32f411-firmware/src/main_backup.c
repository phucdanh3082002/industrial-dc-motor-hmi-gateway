/**
 * @file main.c
 * @brief STM32F411CEU6 - Motor Control Firmware with FreeRTOS
 *
 * Phase 2 + FreeRTOS Integration:
 *   - Motor State Machine (FSM): STOP → RUNNING → WARNING → FAULT
 *   - Over-temperature (45°C warn, 65°C fault) & over-current (1A/2A) protection
 *   - Fault Manager with escalation rules
 *   - FreeRTOS tasks: sensor, motor, uart, watchdog
 *   - IWDG hardware watchdog (2s timeout)
 */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "adc.h"
#include "i2c.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
#include "drivers/gpio_driver.h"
#include "drivers/uart_debug.h"
#include "drivers/i2c_driver.h"
#include "sensors/lm35_test.h"
#include "sensors/ina219_test.h"
#include "motor/motor_control.h"
#include "fault/fault_manager.h"
#include "freertos/freertos_tasks.h"
#include "config/config.h"

#include "FreeRTOS.h"
#include "task.h"

/* Private variables ---------------------------------------------------------*/
IWDG_HandleTypeDef hiwdg;

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);

/* ============================================================================
 * FreeRTOS Hook Functions
 * ============================================================================ */

/**
 * @brief Stack overflow hook - called when task stack overflow detected
 */
void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
    (void)xTask;
    uart_debug_printf("[FATAL] Stack overflow in task: %s\r\n", pcTaskName);
    taskDISABLE_INTERRUPTS();
    for (;;) { }
}

/**
 * @brief Malloc failed hook - called when pvPortMalloc() fails
 */
void vApplicationMallocFailedHook(void)
{
    uart_debug_printf("[FATAL] FreeRTOS malloc failed! Heap exhausted.\r\n");
    taskDISABLE_INTERRUPTS();
    for (;;) { }
}

/**
 * @brief Tick hook - called every FreeRTOS tick (1ms)
 * Used to call HAL_IncTick() for HAL_Delay() compatibility
 */
void vApplicationTickHook(void)
{
    HAL_IncTick();
}

/* ============================================================================
 * Main Entry Point
 * ============================================================================ */

int main(void)
{
    /* --- HAL + Peripheral Init --- */
    HAL_Init();
    SystemClock_Config();

    MX_GPIO_Init();
    MX_ADC1_Init();
    MX_I2C1_Init();
    MX_TIM1_Init();
    MX_USART1_UART_Init();
    MX_USART2_UART_Init();

    /* --- Driver Init --- */
    gpio_driver_init();
    uart_debug_init();

    uart_debug_printf("\r\n");
    uart_debug_printf("============================================\r\n");
    uart_debug_printf("  STM32F411CEU6 - Motor Control Firmware\r\n");
    uart_debug_printf("  Build: %s %s\r\n", __DATE__, __TIME__);
    uart_debug_printf("  Phase 2 + FreeRTOS Integration\r\n");
    uart_debug_printf("============================================\r\n");
    uart_debug_printf("SYSCLK: %lu MHz\r\n", HAL_RCC_GetSysClockFreq() / 1000000UL);
    uart_debug_printf("Chip ID: 0x%08lX\r\n", HAL_GetDEVID());

    /* --- Motor Control Init --- */
    uart_debug_printf("[INIT] Starting motor control initialization...\r\n");
    if (motor_control_init() != 0) {
        uart_debug_printf("[INIT] Motor control init FAILED!\r\n");
        Error_Handler();
    }

    /* --- TIM1 Register Diagnostic --- */
    uart_debug_printf("[DIAG] TIM1: CR1=0x%04lX CCER=0x%04lX BDTR=0x%04lX ARR=%lu\r\n",
                      (unsigned long)htim1.Instance->CR1,
                      (unsigned long)htim1.Instance->CCER,
                      (unsigned long)htim1.Instance->BDTR,
                      (unsigned long)htim1.Instance->ARR);

    /* --- Enable IWDG (2s timeout) --- */
    /* IWDG clock = LSI 40kHz, Prescaler 64 → 625Hz, Reload 1250 → 2s */
    hiwdg.Instance = IWDG;
    hiwdg.Init.Prescaler = IWDG_PRESCALER_64;
    hiwdg.Init.Reload = 1250U;
    if (HAL_IWDG_Init(&hiwdg) != HAL_OK) {
        uart_debug_printf("[INIT] IWDG init FAILED!\r\n");
    } else {
        uart_debug_printf("[INIT] IWDG enabled (2s timeout)\r\n");
    }

    /* --- FreeRTOS: Create objects + tasks + start scheduler --- */
    uart_debug_printf("[INIT] Initializing FreeRTOS...\r\n");
    freertos_objects_init();
    freertos_tasks_init();

    /* Should never reach here - scheduler takes over */
    for (;;) { }
}

/* ============================================================================
 * System Clock: 84 MHz (HSE 25MHz → PLL → SYSCLK)
 * ============================================================================ */

void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    __HAL_RCC_PWR_CLK_ENABLE();
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState = RCC_HSE_ON;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLM = 25;
    RCC_OscInitStruct.PLL.PLLN = 336;
    RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV4;
    RCC_OscInitStruct.PLL.PLLQ = 4;
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
        Error_Handler();
    }

    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
                                | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK) {
        Error_Handler();
    }
}

/* ============================================================================
 * Error Handler
 * ============================================================================ */

void Error_Handler(void)
{
    __disable_irq();
    while (1) { }
}

#ifdef USE_FULL_ASSERT
void assert_failed(uint8_t *file, uint32_t line)
{
    (void)file;
    (void)line;
}
#endif
