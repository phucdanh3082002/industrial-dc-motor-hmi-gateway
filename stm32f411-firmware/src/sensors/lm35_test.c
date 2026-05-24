/**
 * @file lm35_test.c
 * @brief LM35 bring-up helper implementation.
 *
 * LM35 output: 10mV per °C, linear from 0°C to 150°C (single supply).
 * ADC formula: voltage_mV = (raw × VREF_mV) / 4095
 * Temperature: temp_c10 = voltage_mV (since 10mV/°C → mV = °C×10)
 *
 * NOTE: ADC sample time should be ≥ 84 cycles for accurate LM35 readings.
 *       Current CubeMX default is 3 cycles (OK for low-impedance sources).
 *       Change in CubeMX: ADC1 → Channel 0 → Sampling Time = 84 Cycles.
 */

#include "lm35_test.h"
#include "../config/config.h"
#include "../drivers/adc_driver.h"

int lm35_test_read_temp_c10(int16_t *ps16_temp_c10)
{
    uint16_t u16_avg_raw = 0U;
    uint32_t u32_mv;

    if (ps16_temp_c10 == NULL) {
        return -1;
    }

    if (adc_driver_read_avg(LM35_TEST_SAMPLES_DEFAULT, &u16_avg_raw) != 0) {
        return -1;
    }

    /* Convert ADC raw to millivolts:
     * voltage_mV = (raw × 3300) / 4095
     * For LM35: 10mV/°C → voltage_mV = temperature × 10
     * So: temp_c10 = voltage_mV (direct mapping)
     */
    u32_mv = ((uint32_t)u16_avg_raw * 3300U) / 4095U;
    *ps16_temp_c10 = (int16_t)u32_mv;
    return 0;
}

int lm35_test_read_temp_c(float *pf32_temp_c)
{
    int16_t s16_temp_c10;

    if (pf32_temp_c == NULL) {
        return -1;
    }

    if (lm35_test_read_temp_c10(&s16_temp_c10) != 0) {
        return -1;
    }

    *pf32_temp_c = (float)s16_temp_c10 / 10.0f;
    return 0;
}
