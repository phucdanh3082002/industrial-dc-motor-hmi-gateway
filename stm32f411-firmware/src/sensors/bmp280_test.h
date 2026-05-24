/**
 * @file bmp280_test.h
 * @brief BMP280 bring-up helper APIs.
 */

#ifndef BMP280_TEST_H
#define BMP280_TEST_H

#include <stdint.h>

int bmp280_test_init(void);
int bmp280_test_read_chip_id(uint8_t *pu8_chip_id);
int bmp280_test_read_temp_press(int16_t *ps16_temp_c10, uint16_t *pu16_press_hpa);

#endif // BMP280_TEST_H
