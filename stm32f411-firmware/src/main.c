/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body - Hardware Test Harness
  *
  * STM32F411CEU6 Black Pill - Phase 1 Hardware Verification
  * Tests: LED, Buzzer, UART Debug, ADC/LM35, I2C Scan, INA219, BMP280, PWM Motor
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "adc.h"
#include "i2c.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "drivers/gpio_driver.h"
#include "drivers/uart_debug.h"
#include "drivers/i2c_driver.h"
#include "sensors/lm35_test.h"
#include "sensors/ina219_test.h"
#include "sensors/bmp280_test.h"
#include "motor/motor_control.h"
#include "fault/fault_manager.h"
#include <stdio.h>
#include <string.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
static volatile uint32_t gu32_test_pass = 0U;
static volatile uint32_t gu32_test_fail = 0U;
static volatile uint32_t gu32_systick_ms = 0U;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/**
 * @brief  Increment millisecond counter (called from SysTick)
 */
void HAL_SYSTICK_Callback(void)
{
    gu32_systick_ms++;
}

/**
 * @brief  Get elapsed milliseconds since boot
 */
static uint32_t test_get_ms(void)
{
    return gu32_systick_ms;
}

/**
 * @brief  Print test result line
 */
static void test_result(const char *pc_name, int s32_result)
{
    if (s32_result == 0) {
        uart_debug_printf("  [PASS] %s\r\n", pc_name);
        gu32_test_pass++;
    } else {
        uart_debug_printf("  [FAIL] %s (error=%d)\r\n", pc_name, s32_result);
        gu32_test_fail++;
    }
}

/**
 * @brief  Print section header
 */
static void test_section(const char *pc_section)
{
    uart_debug_printf("\r\n--- %s ---\r\n", pc_section);
}

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_ADC1_Init();
  MX_I2C1_Init();
  MX_TIM1_Init();
  MX_USART1_UART_Init();
  MX_USART2_UART_Init();
  /* USER CODE BEGIN 2 */

  /* ========================================================================
   * PHASE 1: BASIC PERIPHERAL INIT
   * ======================================================================== */
  gpio_driver_init();
  uart_debug_init();

  uart_debug_printf("\r\n");
  uart_debug_printf("============================================\r\n");
  uart_debug_printf("  STM32F411CEU6 - Motor Control Firmware\r\n");
  uart_debug_printf("  Build: %s %s\r\n", __DATE__, __TIME__);
  uart_debug_printf("  Phase 2: Motor Control & Protection\r\n");
  uart_debug_printf("============================================\r\n");
  uart_debug_printf("SYSCLK: %lu MHz\r\n", HAL_RCC_GetSysClockFreq() / 1000000UL);
  uart_debug_printf("Chip ID: 0x%08lX\r\n", HAL_GetDEVID());

  /* ========================================================================
   * PHASE 2: MOTOR CONTROL INIT
   * ======================================================================== */
  uart_debug_printf("\r\n[INIT] Starting motor control initialization...\r\n");

  if (motor_control_init() != 0) {
      uart_debug_printf("[INIT] Motor control init FAILED!\r\n");
      Error_Handler();
  }

  /* Start with motor stopped, manual mode, default thresholds */
  motor_command_t s_motor_cmd;
  s_motor_cmd.u8_enable         = 0U;
  s_motor_cmd.u8_pwm_setpoint   = 0U;
  s_motor_cmd.e_mode            = MOTOR_MODE_MANUAL;
  s_motor_cmd.u8_reset_fault    = 0U;
  s_motor_cmd.u16_current_limit = 1000U;    /* 1.0A */
  s_motor_cmd.u16_temp_warn     = 450U;     /* 45.0°C */
  s_motor_cmd.u16_temp_fault    = 650U;     /* 65.0°C */

  uart_debug_printf("[INIT] Motor control ready. Starting super loop...\r\n");
  uart_debug_printf("[INIT] Send 'S' to start motor, 'X' to stop, 'R' to reset fault\r\n");
  uart_debug_printf("[INIT] Send '1'-'9' to set PWM (10%%-90%%), '0' for 100%%\r\n");
  uart_debug_printf("[INIT] Send 'A' for auto mode, 'M' for manual mode\r\n\r\n");

  /* ========================================================================
   * SUPER LOOP: Motor Control + UART Command Parser
   * ======================================================================== */
  uint32_t u32_last_motor_update = 0U;
  uint32_t u32_last_status_print = 0U;
  uint32_t u32_last_watchdog_feed = 0U;
  uint8_t u8_rx_byte = 0U;

  /* Enable IWDG (Independent Watchdog) - 2s timeout */
  /* NOTE: Uncomment after verifying IWDG works in test
  IWDG_HandleTypeDef hiwdg;
  hiwdg.Instance = IWDG;
  hiwdg.Init.Prescaler = IWDG_PRESCALER_4;
  hiwdg.Init.Reload = 4095U;  // ~2s timeout at 40kHz/4
  HAL_IWDG_Init(&hiwdg);
  */

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */

    uint32_t u32_now = test_get_ms();

    /* --- 1. Motor control update (every 100ms) --- */
    if ((u32_now - u32_last_motor_update) >= 100U) {
        u32_last_motor_update = u32_now;
        motor_control_update(&s_motor_cmd);
    }

    /* --- 2. Status print (every 2 seconds) --- */
    if ((u32_now - u32_last_status_print) >= 2000U) {
        u32_last_status_print = u32_now;

        const motor_sensor_data_t *ps_sensor = motor_get_sensor_data();
        uart_debug_printf("--- Status ---\r\n");
        uart_debug_printf("  State: %d | PWM: %d%% | Mode: %s\r\n",
                          motor_get_state(), motor_get_pwm(),
                          (s_motor_cmd.e_mode == MOTOR_MODE_MANUAL) ? "MANUAL" : "AUTO");
        uart_debug_printf("  Temp: %.1f°C | Current: %dmA | Voltage: %dmV\r\n",
                          (float)ps_sensor->s16_lm35_temp_c10 / 10.0f,
                          ps_sensor->s16_current_ma,
                          ps_sensor->u16_voltage_mv);
        uart_debug_printf("  Alarm: 0x%04X | Fault: %d | Errors: 0x%02X\r\n",
                          motor_get_alarm_flags(),
                          motor_get_fault_code(),
                          ps_sensor->u8_sensor_errors);
        uart_debug_printf("---\r\n");
    }

    /* --- 3. Watchdog feed (every 1 second) --- */
    if ((u32_now - u32_last_watchdog_feed) >= 1000U) {
        u32_last_watchdog_feed = u32_now;
        /* HAL_IWDG_Refresh(&hiwdg);  // Uncomment when IWDG is enabled */
    }

    /* --- 4. UART command parser (non-blocking) --- */
    if (HAL_UART_Receive(&huart1, &u8_rx_byte, 1U, 0U) == HAL_OK) {
        switch (u8_rx_byte) {
            case 'S':   /* Start motor */
            case 's':
                s_motor_cmd.u8_enable = 1U;
                s_motor_cmd.u8_pwm_setpoint = 50U;  /* Default 50% */
                uart_debug_printf("[CMD] Motor START (50%%)\r\n");
                break;

            case 'X':   /* Stop motor */
            case 'x':
                s_motor_cmd.u8_enable = 0U;
                s_motor_cmd.u8_pwm_setpoint = 0U;
                uart_debug_printf("[CMD] Motor STOP\r\n");
                break;

            case 'R':   /* Reset fault */
            case 'r':
                s_motor_cmd.u8_reset_fault = 1U;
                uart_debug_printf("[CMD] Reset fault\r\n");
                HAL_Delay(100U);
                s_motor_cmd.u8_reset_fault = 0U;
                break;

            case '0':   /* 100% PWM */
                s_motor_cmd.u8_pwm_setpoint = 100U;
                uart_debug_printf("[CMD] PWM = 100%%\r\n");
                break;

            case '1':   /* 10% PWM */
            case '2':   /* 20% */
            case '3':   /* 30% */
            case '4':   /* 40% */
            case '5':   /* 50% */
            case '6':   /* 60% */
            case '7':   /* 70% */
            case '8':   /* 80% */
            case '9':   /* 90% */
                s_motor_cmd.u8_pwm_setpoint = (u8_rx_byte - '0') * 10U;
                uart_debug_printf("[CMD] PWM = %d%%\r\n", s_motor_cmd.u8_pwm_setpoint);
                break;

            case 'A':   /* Auto mode */
            case 'a':
                s_motor_cmd.e_mode = MOTOR_MODE_AUTO;
                s_motor_cmd.u8_enable = 1U;
                uart_debug_printf("[CMD] Mode -> AUTO\r\n");
                break;

            case 'M':   /* Manual mode */
            case 'm':
                s_motor_cmd.e_mode = MOTOR_MODE_MANUAL;
                uart_debug_printf("[CMD] Mode -> MANUAL\r\n");
                break;

            default:
                uart_debug_printf("[CMD] Unknown: '%c' (0x%02X)\r\n",
                                  (char)u8_rx_byte, u8_rx_byte);
                break;
        }
    }
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 25;
  RCC_OscInitStruct.PLL.PLLN = 336;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV4;
  RCC_OscInitStruct.PLL.PLLQ = 4;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
