/**
 * @file adc_driver.h
 * @brief ADC helper cho LM35 trên ADC1_IN0.
 */

#ifndef ADC_DRIVER_H
#define ADC_DRIVER_H

#include <stdint.h>

int adc_driver_read_raw(uint16_t *pu16_raw);
int adc_driver_read_avg(uint8_t u8_samples, uint16_t *pu16_avg);

#endif // ADC_DRIVER_H
