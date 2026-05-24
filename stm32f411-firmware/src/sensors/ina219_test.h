/**
 * @file ina219_test.h
 * @brief INA219 bring-up helper APIs.
 */

#ifndef INA219_TEST_H
#define INA219_TEST_H

#include <stdint.h>

int ina219_test_init(void);
int ina219_test_read_bus_voltage_mv(uint16_t *pu16_voltage_mv);
int ina219_test_read_shunt_voltage_mv(int16_t *ps16_shunt_mv);
int ina219_test_read_current_ma(int16_t *ps16_current_ma);

#endif // INA219_TEST_H
