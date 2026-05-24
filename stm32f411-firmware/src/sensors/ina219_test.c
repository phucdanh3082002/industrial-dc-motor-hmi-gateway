/**
 * @file ina219_test.c
 * @brief INA219 bring-up helper implementation.
 *
 * INA219 I2C address: 0x40 (SDO/GND) or 0x41 (SDO/VCC).
 * Calibration assumes:
 *   - Shunt resistor: 0.1Ω (100mΩ) → ±3.2A range
 *   - Bus voltage range: 32V
 *   - Current LSB: 100µA (0.1mA per bit)
 *   - Cal register = trunc(0.04096 / (0.0001 × 0.1)) = 4096 (0x1000)
 *
 * IMPORTANT: If using a different shunt resistor value, recalculate:
 *   Cal = trunc(0.04096 / (Current_LSB × R_shunt))
 *   Current_LSB = max_expected_current / 2^15
 */

#include "ina219_test.h"
#include "../config/config.h"
#include "../drivers/i2c_driver.h"

#define INA219_TIMEOUT_MS        50U
#define INA219_REG_CONFIG        0x00U
#define INA219_REG_SHUNT_VOLT    0x01U
#define INA219_REG_BUS_VOLT      0x02U
#define INA219_REG_CURRENT       0x04U
#define INA219_REG_CALIB         0x05U

#define INA219_CFG_32V_2A_CONT   0x399FU
#define INA219_CAL_32V_2A        0x1000U

static int ina219_write_u16(uint8_t u8_reg, uint16_t u16_val)
{
    uint8_t au8_data[2];

    au8_data[0] = (uint8_t)(u16_val >> 8U);
    au8_data[1] = (uint8_t)(u16_val & 0xFFU);

    return i2c_driver_write_reg(INA219_ADDR, u8_reg, au8_data, 2U, INA219_TIMEOUT_MS);
}

static int ina219_read_u16(uint8_t u8_reg, uint16_t *pu16_val)
{
    uint8_t au8_data[2];

    if (pu16_val == NULL) {
        return -1;
    }

    if (i2c_driver_read_reg(INA219_ADDR, u8_reg, au8_data, 2U, INA219_TIMEOUT_MS) != 0) {
        return -1;
    }

    *pu16_val = (uint16_t)(((uint16_t)au8_data[0] << 8U) | (uint16_t)au8_data[1]);
    return 0;
}

int ina219_test_init(void)
{
    if (ina219_write_u16(INA219_REG_CONFIG, INA219_CFG_32V_2A_CONT) != 0) {
        return -1;
    }

    if (ina219_write_u16(INA219_REG_CALIB, INA219_CAL_32V_2A) != 0) {
        return -1;
    }

    return 0;
}

int ina219_test_read_bus_voltage_mv(uint16_t *pu16_voltage_mv)
{
    uint16_t u16_reg;
    uint16_t u16_raw;

    if (pu16_voltage_mv == NULL) {
        return -1;
    }

    if (ina219_read_u16(INA219_REG_BUS_VOLT, &u16_reg) != 0) {
        return -1;
    }

    u16_raw = (uint16_t)((u16_reg >> 3U) & 0x1FFFU);
    *pu16_voltage_mv = (uint16_t)(u16_raw * 4U);
    return 0;
}

int ina219_test_read_shunt_voltage_mv(int16_t *ps16_shunt_mv)
{
    uint16_t u16_reg;
    int16_t s16_raw;

    if (ps16_shunt_mv == NULL) {
        return -1;
    }

    if (ina219_read_u16(INA219_REG_SHUNT_VOLT, &u16_reg) != 0) {
        return -1;
    }

    s16_raw = (int16_t)u16_reg;
    *ps16_shunt_mv = (int16_t)(s16_raw / 100);
    return 0;
}

int ina219_test_read_current_ma(int16_t *ps16_current_ma)
{
    uint16_t u16_reg;
    int16_t s16_raw;

    if (ps16_current_ma == NULL) {
        return -1;
    }

    if (ina219_read_u16(INA219_REG_CURRENT, &u16_reg) != 0) {
        return -1;
    }

    s16_raw = (int16_t)u16_reg;

    if (s16_raw >= 0) {
        *ps16_current_ma = (int16_t)((s16_raw + 5) / 10);
    } else {
        *ps16_current_ma = (int16_t)((s16_raw - 5) / 10);
    }

    return 0;
}
