#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdarg.h>
#include <drivers/uart.h>

static int vprintk(const char *format, va_list ap)
{
    assert(format && *format != '\0');

    static char buffer[512];

    int rc = vsnprintf(buffer, sizeof(buffer), format, ap);
    __uart_write(&uart_dev1, buffer, rc);

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
