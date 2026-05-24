/**
 * @file lm35_test.h
 * @brief LM35 bring-up helper APIs.
 */

#ifndef LM35_TEST_H
#define LM35_TEST_H

#include <stdint.h>

#define LM35_TEST_SAMPLES_DEFAULT   16U

int lm35_test_read_temp_c10(int16_t *ps16_temp_c10);
int lm35_test_read_temp_c(float *pf32_temp_c);

#endif // LM35_TEST_H
