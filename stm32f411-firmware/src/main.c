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
  uart_debug_printf("  STM32F411CEU6 - Hardware Test Harness\r\n");
  uart_debug_printf("  Build: %s %s\r\n", __DATE__, __TIME__);
  uart_debug_printf("============================================\r\n");
  uart_debug_printf("SYSCLK: %lu MHz\r\n", HAL_RCC_GetSysClockFreq() / 1000000UL);
  uart_debug_printf("APB1:   %lu MHz\r\n", HAL_RCC_GetPCLK1Freq() / 1000000UL);
  uart_debug_printf("APB2:   %lu MHz\r\n", HAL_RCC_GetPCLK2Freq() / 1000000UL);
  uart_debug_printf("Chip ID: 0x%08lX\r\n", HAL_GetDEVID());

  /* ========================================================================
   * TEST 1: LED (PC13, Active LOW)
   * ======================================================================== */
  test_section("TEST 1: LED (PC13)");
  uart_debug_printf("  Blinks LED 3 times (500ms interval)...\r\n");

  for (uint8_t i = 0U; i < 3U; i++) {
      gpio_set_led(1U);   /* LED ON (PC13 LOW) */
      HAL_Delay(500);
      gpio_set_led(0U);   /* LED OFF (PC13 HIGH) */
      HAL_Delay(500);
  }
  test_result("LED toggle", 0);

  /* ========================================================================
   * TEST 2: BUZZER (PB12, Active HIGH)
   * ======================================================================== */
  test_section("TEST 2: BUZZER (PB12)");
  uart_debug_printf("  Beep for 200ms...\r\n");

  gpio_set_buzzer(1U);
  HAL_Delay(200);
  gpio_set_buzzer(0U);
  test_result("Buzzer beep", 0);

  /* ========================================================================
   * TEST 3: UART DEBUG (USART1, PA9/PA10, 115200)
   * ======================================================================== */
  test_section("TEST 3: UART DEBUG (USART1)");
  uart_debug_printf("  Loopback test: if you see this, UART OK.\r\n");
  uart_debug_printf("  Baudrate: 115200, 8N1\r\n");
  test_result("UART debug print", 0);

  /* ========================================================================
   * TEST 4: ADC - LM35 TEMPERATURE (PA0, ADC1_IN0)
   * ======================================================================== */
  test_section("TEST 4: LM35 TEMPERATURE (ADC1_IN0/PA0)");
  {
      int16_t s16_temp_c10;
      float f32_temp_c;
      int s32_ret;

      uart_debug_printf("  Reading LM35 (16 samples average)...\r\n");

      s32_ret = lm35_test_read_temp_c(&f32_temp_c);
      if (s32_ret == 0) {
          s16_temp_c10 = (int16_t)(f32_temp_c * 10.0f);
          uart_debug_printf("  Temperature: %.1f C (reg: %d)\r\n", f32_temp_c, s16_temp_c10);
          uart_debug_printf("  ADC raw avg: check via debugger\r\n");
      }
      test_result("LM35 temperature read", s32_ret);
  }

  /* ========================================================================
   * TEST 5: I2C SCAN (PB6=SCL, PB7=SDA, 100kHz)
   * ======================================================================== */
  test_section("TEST 5: I2C SCAN (I2C1)");
  {
      uint8_t au8_found[8];
      uint8_t u8_count = 0U;
      int s32_ret;

      s32_ret = i2c_driver_scan(au8_found, 8U, &u8_count);
      if (s32_ret == 0) {
          uart_debug_printf("  Found %d device(s):", u8_count);
          for (uint8_t i = 0U; i < u8_count; i++) {
              uart_debug_printf(" 0x%02X", au8_found[i]);
          }
          uart_debug_printf("\r\n");
          uart_debug_printf("  Expected: 0x40 (INA219), 0x76 (BMP280)\r\n");
      }
      test_result("I2C bus scan", s32_ret);
  }

  /* ========================================================================
   * TEST 6: INA219 CURRENT/VOLTAGE SENSOR (I2C addr 0x40)
   * ======================================================================== */
  test_section("TEST 6: INA219 (I2C 0x40)");
  {
      int16_t s16_current_ma;
      uint16_t u16_voltage_mv;
      int s32_ret;

      uart_debug_printf("  Initializing INA219 (32V, 2A range)...\r\n");
      s32_ret = ina219_test_init();

      if (s32_ret == 0) {
          HAL_Delay(10U);

          s32_ret = ina219_test_read_bus_voltage_mv(&u16_voltage_mv);
          if (s32_ret == 0) {
              uart_debug_printf("  Bus voltage: %u mV (%.2f V)\r\n",
                                u16_voltage_mv, (float)u16_voltage_mv / 1000.0f);
          } else {
              uart_debug_printf("  [WARN] Bus voltage read failed\r\n");
          }

          s32_ret = ina219_test_read_current_ma(&s16_current_ma);
          if (s32_ret == 0) {
              uart_debug_printf("  Current: %d mA\r\n", s16_current_ma);
          } else {
              uart_debug_printf("  [WARN] Current read failed\r\n");
          }
      }
      test_result("INA219 init + read", s32_ret);
  }

  /* ========================================================================
   * TEST 7: BMP280 TEMPERATURE/PRESSURE (I2C addr 0x76)
   * ======================================================================== */
  test_section("TEST 7: BMP280 (I2C 0x76)");
  {
      uint8_t u8_chip_id = 0U;
      int16_t s16_temp_c10;
      uint16_t u16_press_hpa;
      int s32_ret;

      uart_debug_printf("  Initializing BMP280...\r\n");
      s32_ret = bmp280_test_init();

      if (s32_ret == 0) {
          bmp280_test_read_chip_id(&u8_chip_id);
          uart_debug_printf("  Chip ID: 0x%02X (expected 0x58)\r\n", u8_chip_id);

          HAL_Delay(50U);

          s32_ret = bmp280_test_read_temp_press(&s16_temp_c10, &u16_press_hpa);
          if (s32_ret == 0) {
              uart_debug_printf("  Temperature: %d (reg 40002: %.1f C)\r\n",
                                s16_temp_c10, (float)s16_temp_c10 / 10.0f);
              uart_debug_printf("  Pressure:    %u hPa (reg 40003)\r\n", u16_press_hpa);
          } else {
              uart_debug_printf("  [WARN] BMP280 read failed\r\n");
          }
      } else if (s32_ret == -2) {
          uart_debug_printf("  [WARN] Wrong chip ID - check wiring!\r\n");
      }
      test_result("BMP280 init + read", s32_ret);
  }

  /* ========================================================================
   * TEST 8: PWM MOTOR OUTPUT (TIM1_CH1, PA8, 20kHz)
   * ======================================================================== */
  test_section("TEST 8: PWM MOTOR (TIM1_CH1/PA8)");
  {
      /* Start PWM output on TIM1 CH1 */
      HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);

      uart_debug_printf("  PWM started: 20kHz, Period=99\r\n");

      /* Sweep: 0% -> 50% -> 100% -> 0% */
      uart_debug_printf("  0%% duty (OFF)...\r\n");
      __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, 0U);
      HAL_Delay(500);

      uart_debug_printf("  50%% duty...\r\n");
      __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, 50U);
      HAL_Delay(500);

      uart_debug_printf("  100%% duty (FULL)...\r\n");
      __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, 99U);
      HAL_Delay(500);

      /* Stop motor */
      __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, 0U);
      HAL_TIM_PWM_Stop(&htim1, TIM_CHANNEL_1);

      uart_debug_printf("  PWM stopped.\r\n");
      test_result("PWM motor sweep", 0);
  }

  /* ========================================================================
   * TEST SUMMARY
   * ======================================================================== */
  uart_debug_printf("\r\n");
  uart_debug_printf("============================================\r\n");
  uart_debug_printf("  TEST SUMMARY\r\n");
  uart_debug_printf("  PASS: %lu / %lu\r\n", gu32_test_pass, (gu32_test_pass + gu32_test_fail));
  uart_debug_printf("  FAIL: %lu\r\n", gu32_test_fail);
  uart_debug_printf("============================================\r\n");

  if (gu32_test_fail == 0U) {
      uart_debug_printf("  ALL TESTS PASSED!\r\n");
  } else {
      uart_debug_printf("  SOME TESTS FAILED - CHECK WIRING!\r\n");
  }
  uart_debug_printf("\r\n");

  /* ========================================================================
   * PERIODIC STATUS LOOP (after all tests pass)
   * ======================================================================== */
  uint32_t u32_last_print = test_get_ms();
  uint32_t u32_interval = 2000U;  /* Print every 2 seconds */

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
    if ((test_get_ms() - u32_last_print) >= u32_interval) {
        u32_last_print = test_get_ms();

        /* Read LM35 */
        float f32_temp;
        if (lm35_test_read_temp_c(&f32_temp) == 0) {
            uart_debug_printf("[LM35]  %.1f C\r\n", f32_temp);
        }

        /* Read INA219 */
        uint16_t u16_mv;
        int16_t s16_ma;
        if (ina219_test_read_bus_voltage_mv(&u16_mv) == 0) {
            uart_debug_printf("[INA219] %u mV / ", u16_mv);
        }
        if (ina219_test_read_current_ma(&s16_ma) == 0) {
            uart_debug_printf("%d mA\r\n", s16_ma);
        } else {
            uart_debug_printf("\r\n");
        }

        /* Read BMP280 */
        int16_t s16_bmp_t;
        uint16_t u16_bmp_p;
        if (bmp280_test_read_temp_press(&s16_bmp_t, &u16_bmp_p) == 0) {
            uart_debug_printf("[BMP280] %.1f C / %u hPa\r\n",
                              (float)s16_bmp_t / 10.0f, u16_bmp_p);
        }

        uart_debug_printf("---\r\n");

        /* Toggle LED as heartbeat */
        gpio_set_led(1U);
        HAL_Delay(50);
        gpio_set_led(0U);
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
