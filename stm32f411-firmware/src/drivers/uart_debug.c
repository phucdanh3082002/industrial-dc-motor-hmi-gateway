/**
 * @file uart_debug.c
 * @brief UART debug helper implementation.
 */

#include "uart_debug.h"
#include "../config/config.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#define UART_DEBUG_TIMEOUT_MS   100U
#define UART_DEBUG_BUF_SIZE     192U

void uart_debug_init(void)
{
    (void)uart_debug_print("[DBG] UART debug ready\r\n");
}

int uart_debug_write(const uint8_t *pu8_data, uint16_t u16_len)
{
    if ((pu8_data == NULL) || (u16_len == 0U)) {
        return -1;
    }

    if (HAL_UART_Transmit(DEBUG_UART, (uint8_t *)pu8_data, u16_len, UART_DEBUG_TIMEOUT_MS)
        != HAL_OK) {
        return -1;
    }

    return 0;
}

int uart_debug_print(const char *pc_msg)
{
    uint16_t u16_len;

    if (pc_msg == NULL) {
        return -1;
    }

    u16_len = (uint16_t)strlen(pc_msg);
    return uart_debug_write((const uint8_t *)pc_msg, u16_len);
}

int uart_debug_printf(const char *pc_fmt, ...)
{
    char c_buf[UART_DEBUG_BUF_SIZE];
    int s32_n;
    va_list st_args;

    if (pc_fmt == NULL) {
        return -1;
    }

    va_start(st_args, pc_fmt);
    s32_n = vsnprintf(c_buf, sizeof(c_buf), pc_fmt, st_args);
    va_end(st_args);

    if (s32_n <= 0) {
        return -1;
    }

    if ((uint32_t)s32_n >= sizeof(c_buf)) {
        c_buf[sizeof(c_buf) - 3U] = '\r';
        c_buf[sizeof(c_buf) - 2U] = '\n';
        c_buf[sizeof(c_buf) - 1U] = '\0';
    }

    return uart_debug_print(c_buf);
}
