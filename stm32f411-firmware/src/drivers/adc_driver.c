/**
 * @file adc_driver.c
 * @brief ADC helper implementation.
 */

#include "adc_driver.h"
#include "../config/config.h"

#define ADC_READ_TIMEOUT_MS    20U

int adc_driver_read_raw(uint16_t *pu16_raw)
{
    if (pu16_raw == NULL) {
        return -1;
    }

    if (HAL_ADC_Start(LM35_ADC) != HAL_OK) {
        return -1;
    }

    if (HAL_ADC_PollForConversion(LM35_ADC, ADC_READ_TIMEOUT_MS) != HAL_OK) {
        (void)HAL_ADC_Stop(LM35_ADC);
        return -1;
    }

    *pu16_raw = (uint16_t)HAL_ADC_GetValue(LM35_ADC);

    if (HAL_ADC_Stop(LM35_ADC) != HAL_OK) {
        return -1;
    }

    return 0;
}

int adc_driver_read_avg(uint8_t u8_samples, uint16_t *pu16_avg)
{
    uint32_t u32_sum = 0U;
    uint16_t u16_raw = 0U;
    uint8_t u8_i;

    if ((pu16_avg == NULL) || (u8_samples == 0U)) {
        return -1;
    }

    for (u8_i = 0U; u8_i < u8_samples; u8_i++) {
        if (adc_driver_read_raw(&u16_raw) != 0) {
            return -1;
        }
        u32_sum += (uint32_t)u16_raw;
    }

    *pu16_avg = (uint16_t)(u32_sum / (uint32_t)u8_samples);
    return 0;
}
