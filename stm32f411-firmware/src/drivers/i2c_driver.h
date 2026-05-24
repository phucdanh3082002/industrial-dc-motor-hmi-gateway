/**
 * @file i2c_driver.h
 * @brief I2C helper cho INA219 và BMP280.
 */

#ifndef I2C_DRIVER_H
#define I2C_DRIVER_H

#include <stdint.h>

int i2c_driver_read_reg(uint16_t u16_addr_8bit,
                        uint8_t u8_reg,
                        uint8_t *pu8_data,
                        uint16_t u16_len,
                        uint32_t u32_timeout_ms);

int i2c_driver_write_reg(uint16_t u16_addr_8bit,
                         uint8_t u8_reg,
                         const uint8_t *pu8_data,
                         uint16_t u16_len,
                         uint32_t u32_timeout_ms);

int i2c_driver_scan(uint8_t *pu8_found_7bit, uint8_t u8_max_count, uint8_t *pu8_count);

#endif // I2C_DRIVER_H
