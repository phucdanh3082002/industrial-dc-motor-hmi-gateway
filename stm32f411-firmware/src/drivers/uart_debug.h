/**
 * @file uart_debug.h
 * @brief UART debug print helper (USART1).
 */

#ifndef UART_DEBUG_H
#define UART_DEBUG_H

#include <stdint.h>

void uart_debug_init(void);
int uart_debug_write(const uint8_t *pu8_data, uint16_t u16_len);
int uart_debug_print(const char *pc_msg);
int uart_debug_printf(const char *pc_fmt, ...);

#endif // UART_DEBUG_H
