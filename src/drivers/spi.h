#ifndef __SPI_H
#define __SPI_H

#include "stm32f1xx_ll_spi.h"

void SPI_cs_sel(SPI_TypeDef *SPIx);
void SPI_cs_desel(SPI_TypeDef *SPIx);
uint8_t SPI_read_byte(SPI_TypeDef *SPIx);
void SPI_write_byte(SPI_TypeDef *SPIx, uint8_t byte);
int SPI_read(SPI_TypeDef *SPIx, void *buf, uint32_t len);
int SPI_write(SPI_TypeDef *SPIx, const void *buf, uint32_t len);

void spi_init(void);

#endif
