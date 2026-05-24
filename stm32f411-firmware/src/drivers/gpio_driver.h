/**
 * @file gpio_driver.h
 * @brief GPIO helper cho LED, buzzer và RS-485 direction.
 */

#ifndef GPIO_DRIVER_H
#define GPIO_DRIVER_H

#include <stdint.h>

void gpio_driver_init(void);
void gpio_set_led(uint8_t u8_on);
void gpio_set_buzzer(uint8_t u8_on);
void gpio_set_rs485_dir(uint8_t u8_tx_enable);

#endif // GPIO_DRIVER_H
