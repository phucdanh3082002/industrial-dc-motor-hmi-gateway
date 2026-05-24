/**
 * @file i2c_driver.c
 * @brief I2C helper implementation.
 */

#include "i2c_driver.h"
#include "../config/config.h"

int i2c_driver_read_reg(uint16_t u16_addr_8bit,
                        uint8_t u8_reg,
                        uint8_t *pu8_data,
                        uint16_t u16_len,
                        uint32_t u32_timeout_ms)
{
    if ((pu8_data == NULL) || (u16_len == 0U)) {
        return -1;
    }

    if (HAL_I2C_Mem_Read(SENSOR_I2C,
                         u16_addr_8bit,
                         u8_reg,
                         I2C_MEMADD_SIZE_8BIT,
                         pu8_data,
                         u16_len,
                         u32_timeout_ms) != HAL_OK) {
        return -1;
    }

    return 0;
}

int i2c_driver_write_reg(uint16_t u16_addr_8bit,
                         uint8_t u8_reg,
                         const uint8_t *pu8_data,
                         uint16_t u16_len,
                         uint32_t u32_timeout_ms)
{
    if ((pu8_data == NULL) || (u16_len == 0U)) {
        return -1;
    }

    if (HAL_I2C_Mem_Write(SENSOR_I2C,
                          u16_addr_8bit,
                          u8_reg,
                          I2C_MEMADD_SIZE_8BIT,
                          (uint8_t *)pu8_data,
                          u16_len,
                          u32_timeout_ms) != HAL_OK) {
        return -1;
    }

    return 0;
}

int i2c_driver_scan(uint8_t *pu8_found_7bit, uint8_t u8_max_count, uint8_t *pu8_count)
{
    uint8_t u8_addr;
    uint8_t u8_found = 0U;

    if ((pu8_count == NULL) || ((pu8_found_7bit == NULL) && (u8_max_count > 0U))) {
        return -1;
    }

    for (u8_addr = 0x08U; u8_addr <= 0x77U; u8_addr++) {
        if (HAL_I2C_IsDeviceReady(SENSOR_I2C, (uint16_t)(u8_addr << 1U), 2U, 20U) == HAL_OK) {
            if (u8_found < u8_max_count) {
                pu8_found_7bit[u8_found] = u8_addr;
            }
            u8_found++;
        }
    }

    *pu8_count = u8_found;
    return 0;
}
