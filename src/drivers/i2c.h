#ifndef __I2C_H
#define __I2C_H

#define I2C_SET_DEV 0
#define I2C_SET_ADDR 1

void I2C1_write_byte(uint8_t dev, uint8_t addr, uint8_t byte);
uint8_t I2C1_read_byte(uint8_t dev, uint8_t addr);

void i2c_init(void);

#endif
