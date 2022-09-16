#ifndef __UART_H
#define __UART_H

#include <stdint.h>
#include <stddef.h>

struct uart_device;
extern struct uart_device uart_dev1, uart_dev2;

void __uart_write_byte(struct uart_device *device, uint8_t byte);
int __uart_write(struct uart_device *device, const void *buf, size_t len);
int __uart_read(struct uart_device *device, void *buf, size_t len);

void uart_init(void);

#endif
