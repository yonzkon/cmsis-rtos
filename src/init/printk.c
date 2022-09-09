#include "stm32f1xx_ll_usart.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdarg.h>

static int vprintk(const char *format, va_list ap)
{
    assert(format && *format != '\0');

    static char buffer[512];

    int rc = vsnprintf(buffer, sizeof(buffer), format, ap);
    if (rc) {
        for (int i = 0; i < rc; i++) {
            while (!LL_USART_IsActiveFlag_TXE(USART1));
            LL_USART_TransmitData8(USART1, ((uint8_t *)buffer)[i]);
        }
    }

    return -1;
}

void printk(const char *format, ...)
{
    assert(format && *format != '\0');

    va_list ap;
    va_start(ap, format);
    vprintk(format, ap);
    va_end(ap);
}

void panic(const char *format, ...)
{
    assert(format && *format != '\0');

    va_list ap;
    va_start(ap, format);
    vprintk(format, ap);
    va_end(ap);

    for (;;);
}
