#include "stm32f1xx_hal.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdarg.h>

extern UART_HandleTypeDef huart1;

static char buffer[512];

static int vprintk(const char *format, va_list ap)
{
    assert(format && *format != '\0');

    int rc = vsnprintf(buffer, sizeof(buffer), format, ap);
    if (rc) {
        HAL_UART_Transmit(&huart1, (uint8_t *)buffer, rc, HAL_MAX_DELAY);
        return rc - huart1.TxXferCount;
    }

    return -1;
}

void printk(const char *format, ...)
{
    va_list ap;
    va_start(ap, format);
    vprintk(format, ap);
    va_end(ap);
}

void panic(const char *format, ...)
{
    va_list ap;
    va_start(ap, format);
    vprintk(format, ap);
    va_end(ap);

    for (;;);
}
