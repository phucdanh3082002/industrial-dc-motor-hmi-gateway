/**
 * @file bmp280_test.c
 * @brief BMP280 bring-up helper implementation.
 *
 * BMP280 I2C address: 0x76 (default) or 0x77 (SDO=HIGH).
 * Uses I2C driver for register read/write.
 * Compensation formulas from Bosch BMP280 datasheet Section 8.1.
 */

#include "bmp280_test.h"
#include "../config/config.h"
#include "../drivers/i2c_driver.h"

/* ============================================================================
 * Register Definitions (BMP280 Datasheet)
 * ========================================================================= */
#define BMP280_REG_ID               0xD0U
#define BMP280_REG_RESET            0xE0U
#define BMP280_REG_STATUS           0xF3U
#define BMP280_REG_CTRL_MEAS        0xF4U
#define BMP280_REG_CONFIG           0xF5U
#define BMP280_REG_PRESS_MSB        0xF7U
#define BMP280_REG_TEMP_MSB         0xFAU
#define BMP280_REG_CALIB_00         0x88U

#define BMP280_CHIP_ID_VALUE        0x58U
#define BMP280_RESET_VALUE          0xB6U

/* ctrl_meas register bits */
#define BMP280_OSRS_T_1X            (0x01U << 5)    /* Temperature oversampling x1  */
#define BMP280_OSRS_P_1X            (0x01U << 2)    /* Pressure oversampling x1     */
#define BMP280_OSRS_T_2X            (0x02U << 5)    /* Temperature oversampling x2  */
#define BMP280_OSRS_P_2X            (0x02U << 2)    /* Pressure oversampling x2     */
#define BMP280_MODE_FORCED          0x01U           /* Single measurement, then sleep */
#define BMP280_MODE_NORMAL          0x03U           /* Continuous measurement */

/* Status register bits */
#define BMP28_STATUS_MEASURING      (1U << 3)

#define BMP280_TIMEOUT_MS           100U
#define BMP280_MEAS_TIMEOUT_MS      50U

/* ============================================================================
 * Calibration Data (stored after init)
 * ========================================================================= */
static uint16_t u16_dig_T1;
static int16_t  s16_dig_T2;
static int16_t  s16_dig_T3;
static uint16_t u16_dig_P1;
static int16_t  s16_dig_P2;
static int16_t  s16_dig_P3;
static int16_t  s16_dig_P4;
static int16_t  s16_dig_P5;
static int16_t  s16_dig_P6;
static int16_t  s16_dig_P7;
static int16_t  s16_dig_P8;
static int16_t  s16_dig_P9;

/* ============================================================================
 * Low-level I2C Helpers
 * ========================================================================= */
static int bmp280_read_reg(uint8_t u8_reg, uint8_t *pu8_data)
{
    return i2c_driver_read_reg(BMP280_ADDR, u8_reg, pu8_data, 1U, BMP280_TIMEOUT_MS);
}

static int bmp280_read_regs(uint8_t u8_reg, uint8_t *pu8_data, uint16_t u16_len)
{
    return i2c_driver_read_reg(BMP280_ADDR, u8_reg, pu8_data, u16_len, BMP280_TIMEOUT_MS);
}

static int bmp280_write_reg(uint8_t u8_reg, uint8_t u8_data)
{
    return i2c_driver_write_reg(BMP280_ADDR, u8_reg, &u8_data, 1U, BMP280_TIMEOUT_MS);
}

/* ============================================================================
 * Read All Calibration Data (26 bytes from 0x88)
 * ========================================================================= */
static int bmp280_read_calibration(void)
{
    uint8_t au8_calib[26];

    if (bmp280_read_regs(BMP280_REG_CALIB_00, au8_calib, 26U) != 0) {
        return -1;
    }

    /* Temperature compensation (0x88 - 0x8D) */
    u16_dig_T1 = (uint16_t)((uint16_t)au8_calib[1] << 8U | (uint16_t)au8_calib[0]);
    s16_dig_T2 = (int16_t)((int16_t)au8_calib[3] << 8U | (int16_t)au8_calib[2]);
    s16_dig_T3 = (int16_t)((int16_t)au8_calib[5] << 8U | (int16_t)au8_calib[4]);

    /* Pressure compensation (0x8E - 0x9F) */
    u16_dig_P1 = (uint16_t)((uint16_t)au8_calib[7] << 8U | (uint16_t)au8_calib[6]);
    s16_dig_P2 = (int16_t)((int16_t)au8_calib[9] << 8U | (int16_t)au8_calib[8]);
    s16_dig_P3 = (int16_t)((int16_t)au8_calib[11] << 8U | (int16_t)au8_calib[10]);
    s16_dig_P4 = (int16_t)((int16_t)au8_calib[13] << 8U | (int16_t)au8_calib[12]);
    s16_dig_P5 = (int16_t)((int16_t)au8_calib[15] << 8U | (int16_t)au8_calib[14]);
    s16_dig_P6 = (int16_t)((int16_t)au8_calib[17] << 8U | (uint16_t)au8_calib[16]);
    s16_dig_P7 = (int16_t)((int16_t)au8_calib[19] << 8U | (uint16_t)au8_calib[18]);
    s16_dig_P8 = (int16_t)((int16_t)au8_calib[21] << 8U | (uint16_t)au8_calib[20]);
    s16_dig_P9 = (int16_t)((int16_t)au8_calib[23] << 8U | (uint16_t)au8_calib[22]);

    return 0;
}

/* ============================================================================
 * Compensation Formulas (BMP280 Datasheet Section 8.1)
 *
 * Temperature: returns 0.01°C (e.g. 2534 = 25.34°C)
 * Pressure: returns Q24.8 Pa (shift right 8 for Pa, divide 25600 for hPa)
 * ========================================================================= */
static int32_t bmp280_compensate_temp(int32_t s32_adc_T, int32_t *ps32_t_fine)
{
    int32_t s32_var1;
    int32_t s32_var2;

    s32_var1 = ((((s32_adc_T >> 3) - ((int32_t)u16_dig_T1 << 1))) *
                ((int32_t)s16_dig_T2)) >> 11;
    s32_var2 = (((((s32_adc_T >> 4) - ((int32_t)u16_dig_T1)) *
                  ((s32_adc_T >> 4) - ((int32_t)u16_dig_T1))) >> 12) *
                ((int32_t)s16_dig_T3)) >> 14;

    *ps32_t_fine = s32_var1 + s32_var2;

    return ((*ps32_t_fine * 5) + 128) >> 8;
}

static uint32_t bmp280_compensate_press(int32_t s32_adc_P, int32_t s32_t_fine)
{
    int64_t s64_var1;
    int64_t s64_var2;
    int64_t s64_p;

    s64_var1 = ((int64_t)s32_t_fine) - 128000;
    s64_var2 = s64_var1 * s64_var1 * (int64_t)s16_dig_P6;
    s64_var2 = s64_var2 + ((s64_var1 * (int64_t)s16_dig_P5) << 17);
    s64_var2 = s64_var2 + (((int64_t)s16_dig_P4) << 35);
    s64_var1 = ((s64_var1 * s64_var1 * (int64_t)s16_dig_P3) >> 8) +
               ((s64_var1 * (int64_t)s16_dig_P2) << 12);
    s64_var1 = (((((int64_t)1) << 47) + s64_var1)) *
               ((int64_t)u16_dig_P1) >> 33;

    if (s64_var1 == 0) {
        return 0;
    }

    s64_p = 1048576 - s32_adc_P;
    s64_p = (((s64_p << 31) - s64_var2) * 3125) / s64_var1;
    s64_var1 = (((int64_t)s16_dig_P9) * (s64_p >> 13) * (s64_p >> 13)) >> 25;
    s64_var2 = (((int64_t)s16_dig_P8) * s64_p) >> 19;
    s64_p = ((s64_p + s64_var1 + s64_var2) >> 8) + (((int64_t)s16_dig_P7) << 4);

    return (uint32_t)s64_p;
}

/* ============================================================================
 * Public API
 * ========================================================================= */

int bmp280_test_init(void)
{
    uint8_t u8_chip_id;
    uint8_t u8_ctrl_meas;

    /* Step 1: Verify chip ID */
    if (bmp280_read_reg(BMP280_REG_ID, &u8_chip_id) != 0) {
        return -1;  /* I2C communication error */
    }

    if (u8_chip_id != BMP280_CHIP_ID_VALUE) {
        return -2;  /* Wrong chip ID - device not BMP280 */
    }

    /* Step 2: Software reset */
    if (bmp280_write_reg(BMP280_REG_RESET, BMP280_RESET_VALUE) != 0) {
        return -1;
    }
    HAL_Delay(10U);  /* Wait for reset to complete (datasheet: 2ms, margin) */

    /* Step 3: Read calibration data */
    if (bmp280_read_calibration() != 0) {
        return -1;
    }

    /* Step 4: Configure for forced mode measurement
     * ctrl_meas = osrs_t[7:5] | osrs_p[4:2] | mode[1:0]
     * osrs_t = x1 (001), osrs_p = x1 (001), mode = forced (01)
     */
    u8_ctrl_meas = BMP280_OSRS_T_1X | BMP280_OSRS_P_1X | BMP280_MODE_FORCED;
    if (bmp280_write_reg(BMP280_REG_CTRL_MEAS, u8_ctrl_meas) != 0) {
        return -1;
    }

    return 0;
}

int bmp280_test_read_chip_id(uint8_t *pu8_chip_id)
{
    if (pu8_chip_id == NULL) {
        return -1;
    }

    return bmp280_read_reg(BMP280_REG_ID, pu8_chip_id);
}

int bmp280_test_read_temp_press(int16_t *ps16_temp_c10, uint16_t *pu16_press_hpa)
{
    uint8_t au8_data[6];
    int32_t s32_adc_T;
    int32_t s32_t_fine;
    int32_t s32_temp_001C;
    uint32_t u32_press_q24_8;
    uint8_t u8_ctrl_meas;
    uint8_t u8_status;
    uint16_t u16_timeout;

    if ((ps16_temp_c10 == NULL) || (pu16_press_hpa == NULL)) {
        return -1;
    }

    /* Trigger forced measurement */
    u8_ctrl_meas = BMP280_OSRS_T_1X | BMP280_OSRS_P_1X | BMP280_MODE_FORCED;
    if (bmp280_write_reg(BMP280_REG_CTRL_MEAS, u8_ctrl_meas) != 0) {
        return -1;
    }

    /* Wait for measurement to complete */
    u16_timeout = 0;
    do {
        HAL_Delay(5U);
        if (bmp280_read_reg(BMP280_REG_STATUS, &u8_status) != 0) {
            return -1;
        }
        u16_timeout += 5U;
    } while (((u8_status & BMP28_STATUS_MEASURING) != 0U) &&
             (u16_timeout < BMP280_MEAS_TIMEOUT_MS));

    if (u16_timeout >= BMP280_MEAS_TIMEOUT_MS) {
        return -1;  /* Measurement timeout */
    }

    /* Read raw data: press_msb, press_lsb, press_xlsb, temp_msb, temp_lsb, temp_xlsb */
    if (bmp280_read_regs(BMP280_REG_PRESS_MSB, au8_data, 6U) != 0) {
        return -1;
    }

    /* Construct raw ADC values (20-bit) */
    s32_adc_T = (int32_t)(((uint32_t)au8_data[3] << 12) |
                           ((uint32_t)au8_data[4] << 4)  |
                           ((uint32_t)au8_data[5] >> 4));

    /* Compensate temperature (returns 0.01°C) */
    s32_temp_001C = bmp280_compensate_temp(s32_adc_T, &s32_t_fine);

    /* Convert to °C × 10 for Modbus register 40002 */
    if (s32_temp_001C < 0) {
        *ps16_temp_c10 = (int16_t)((s32_temp_001C - 5) / 10);   /* Round toward zero */
    } else {
        *ps16_temp_c10 = (int16_t)((s32_temp_001C + 5) / 10);   /* Round to nearest */
    }

    /* Compensate pressure (returns Q24.8 Pa) */
    u32_press_q24_8 = bmp280_compensate_press(
        (int32_t)(((uint32_t)au8_data[0] << 12) |
                  ((uint32_t)au8_data[1] << 4)  |
                  ((uint32_t)au8_data[2] >> 4)),
        s32_t_fine
    );

    /* Convert Q24.8 Pa to hPa:
     * pressure_pa = u32_press_q24_8 >> 8
     * pressure_hpa = pressure_pa / 100
     * Combined: pressure_hpa = u32_press_q24_8 / 25600
     */
    *pu16_press_hpa = (uint16_t)(u32_press_q24_8 / 25600U);

    return 0;
}
