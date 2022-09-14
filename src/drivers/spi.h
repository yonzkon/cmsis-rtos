#ifndef __SPI_H
#define __SPI_H

void spi_init(void);

void SPI1_cs_sel(void);
void SPI1_cs_desel(void);
uint8_t SPI1_read_byte();
void SPI1_write_byte(uint8_t byte);
int SPI1_read(void *buf, uint32_t len);
int SPI1_write(const void *buf, uint32_t len);

void SPI2_cs_sel(void);
void SPI2_cs_desel(void);
void SPI2_read_byte();
void SPI2_write_byte(uint8_t byte);
int SPI2_read(void *buf, uint32_t len);
int SPI2_write(const void *buf, uint32_t len);

#endif
