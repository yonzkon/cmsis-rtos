#ifndef __SPI_H
#define __SPI_H

void spi_init(void);

uint8_t SPI1_read_byte(uint8_t byte);
void SPI1_write_byte(uint8_t byte);

#endif
