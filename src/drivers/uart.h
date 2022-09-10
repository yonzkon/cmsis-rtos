#ifndef __UART_H
#define __UART_H

#include <stdint.h>
#include <stddef.h>

void uart_init(void);

void UART1_write_byte(uint8_t byte);
int UART1_write(const void *buf, size_t len);
int UART1_read(void *buf, size_t len);

void UART2_write_byte(uint8_t byte);
int UART2_write(const void *buf, size_t len);
int UART2_read(void *buf, size_t len);

#endif
