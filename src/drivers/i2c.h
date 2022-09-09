#ifndef __I2C_H
#define __I2C_H

void i2c_init(void);

void I2C1_write_byte(uint8_t dev, uint8_t addr, uint8_t byte);
uint8_t I2C1_read_byte(uint8_t dev, uint8_t addr);

#endif
