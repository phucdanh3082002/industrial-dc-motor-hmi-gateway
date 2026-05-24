/**
 * @file gpio_driver.c
 * @brief GPIO helper implementation.
 */

#include "gpio_driver.h"
#include "../config/config.h"

void gpio_driver_init(void)
{
    gpio_set_led(0U);
    gpio_set_buzzer(0U);
    gpio_set_rs485_dir(0U);
}

void gpio_set_led(uint8_t u8_on)
{
    GPIO_PinState e_state = (u8_on != 0U) ? LED_ON : LED_OFF;
    HAL_GPIO_WritePin(LED_PORT, LED_PIN, e_state);
}

void gpio_set_buzzer(uint8_t u8_on)
{
    GPIO_PinState e_state = (u8_on != 0U) ? BUZZER_ON : BUZZER_OFF;
    HAL_GPIO_WritePin(BUZZER_PORT, BUZZER_PIN, e_state);
}

void gpio_set_rs485_dir(uint8_t u8_tx_enable)
{
    GPIO_PinState e_state = (u8_tx_enable != 0U) ? RS485_TX_ENABLE : RS485_RX_ENABLE;
    HAL_GPIO_WritePin(RS485_DIR_PORT, RS485_DIR_PIN, e_state);
}
