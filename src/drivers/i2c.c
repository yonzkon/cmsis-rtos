#include "stm32f1xx_ll_bus.h"
#include "stm32f1xx_ll_gpio.h"
#include "stm32f1xx_ll_i2c.h"
#include "i2c.h"
#include <assert.h>
#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <printk.h>
#include <fs/fs.h>

void I2C_write_byte(I2C_TypeDef *I2Cx, uint8_t dev, uint8_t addr, uint8_t byte)
{
    while (LL_I2C_IsActiveFlag_BUSY(I2Cx));

    //I2Cx->CR1 |= I2C_CR1_START;
    LL_I2C_GenerateStartCondition(I2Cx);
    while (!LL_I2C_IsActiveFlag_SB(I2Cx));

    LL_I2C_TransmitData8(I2Cx, dev << 1);
    while (!LL_I2C_IsActiveFlag_ADDR(I2Cx));
    LL_I2C_ClearFlag_ADDR(I2Cx);

    //I2Cx->DR = 0x10;
    LL_I2C_TransmitData8(I2Cx, addr);
    while (!LL_I2C_IsActiveFlag_TXE(I2Cx));

    //I2Cx->DR = 0xcc;
    LL_I2C_TransmitData8(I2Cx, byte);
    while (!LL_I2C_IsActiveFlag_BTF(I2Cx));

    I2Cx->CR1 |= I2C_CR1_STOP;
}

uint8_t I2C_read_byte(I2C_TypeDef *I2Cx, uint8_t dev, uint8_t addr)
{
    while(LL_I2C_IsActiveFlag_BUSY(I2Cx));

    LL_I2C_AcknowledgeNextData(I2Cx, LL_I2C_ACK);
    LL_I2C_GenerateStartCondition(I2Cx);
    while(!LL_I2C_IsActiveFlag_SB(I2Cx));

    LL_I2C_TransmitData8(I2Cx, dev);
    while(!LL_I2C_IsActiveFlag_ADDR(I2Cx));
    LL_I2C_ClearFlag_ADDR(I2Cx);

    while(!LL_I2C_IsActiveFlag_TXE(I2Cx));
    LL_I2C_TransmitData8(I2Cx, addr);
    while(!LL_I2C_IsActiveFlag_TXE(I2Cx));

    LL_I2C_GenerateStopCondition(I2Cx);
    LL_I2C_GenerateStartCondition(I2Cx);
    while(!LL_I2C_IsActiveFlag_SB(I2Cx));

    LL_I2C_TransmitData8(I2Cx, dev | 0x01);
    while(!LL_I2C_IsActiveFlag_ADDR(I2Cx));

    LL_I2C_AcknowledgeNextData(I2Cx, LL_I2C_NACK);
    LL_I2C_ClearFlag_ADDR(I2Cx);
    LL_I2C_GenerateStopCondition(I2Cx);

    while(!LL_I2C_IsActiveFlag_RXNE(I2Cx));
    return LL_I2C_ReceiveData8(I2Cx);
}

int I2C_write(I2C_TypeDef *I2Cx, uint8_t dev, uint8_t addr, const void *buf, uint32_t len)
{
    for (int i = 0; i < len; i++)
        I2C_write_byte(I2Cx, dev, addr, ((uint8_t *)buf)[len]);
    return len;
}

int I2C_read(I2C_TypeDef *I2Cx, uint8_t dev, uint8_t addr, void *buf, uint32_t len)
{
    for (int i = 0; i < len; i++)
        ((uint8_t *)buf)[len] = I2C_read_byte(I2Cx, dev, addr);
    return len;
}

static struct i2c_device {
    struct inode *inode;
    struct dentry *dentry;
    I2C_TypeDef *i2c;
    uint8_t dev;
    uint8_t addr;
} i2c_dev1;

static int i2c_open(struct file *file)
{
    if (strcmp(file->dentry->name, "i2c1") == 0) {
        file->private_data = &i2c_dev1;
    } else {
        assert(0);
    }
    return 0;
}

static int i2c_close(struct file *file)
{
    return 0;
}

static int i2c_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    struct i2c_device *device = file->private_data;

    if (cmd == I2C_SET_DEV) {
        device->dev = arg;
        return 0;
    } else if (cmd == I2C_SET_ADDR) {
        device->addr = arg;
        return 0;
    }
    errno = EINVAL;
    return -1;
}

static int i2c_write(struct file *file, const void *buf, uint32_t len)
{
    struct i2c_device *device = file->private_data;
    return I2C_write(device->i2c, device->dev, device->addr, buf, len);
}

static int i2c_read(struct file *file, void *buf, uint32_t len)
{
    struct i2c_device *device = file->private_data;
    return I2C_read(device->i2c, device->dev, device->addr, buf, len);
}

static struct file_operations i2c_fops =  {
    .open = i2c_open,
    .close = i2c_close,
    .ioctl = i2c_ioctl,
    .write = i2c_write,
    .read = i2c_read,
};

static void I2C1_init(void)
{
    LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_GPIOB);
    LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_I2C1);

    /*
     * I2C1 GPIO Configuration
     * PB6   ------> I2C1_SCL
     * PB7   ------> I2C1_SDA
     */
    LL_GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = LL_GPIO_PIN_6|LL_GPIO_PIN_7;
    GPIO_InitStruct.Mode = LL_GPIO_MODE_ALTERNATE;
    GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_OPENDRAIN;
    LL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    /*
     * I2C Initialization
     */
    LL_I2C_InitTypeDef I2C_InitStruct = {0};
    LL_I2C_DisableOwnAddress2(I2C1);
    LL_I2C_DisableGeneralCall(I2C1);
    LL_I2C_EnableClockStretching(I2C1);
    I2C_InitStruct.PeripheralMode = LL_I2C_MODE_I2C;
    I2C_InitStruct.ClockSpeed = 100000;
    I2C_InitStruct.DutyCycle = LL_I2C_DUTYCYCLE_2;
    I2C_InitStruct.OwnAddress1 = 0x24;
    I2C_InitStruct.TypeAcknowledge = LL_I2C_ACK;
    I2C_InitStruct.OwnAddrSize = LL_I2C_OWNADDRESS1_7BIT;
    LL_I2C_Init(I2C1, &I2C_InitStruct);
    LL_I2C_SetOwnAddress2(I2C1, 0);
    LL_I2C_Enable(I2C1);

    // fs init
    struct inode *inode = calloc(1, sizeof(*inode));
    inode->type = INODE_TYPE_CHAR;
    inode->f_ops = i2c_fops;
    INIT_LIST_HEAD(&inode->node);
    struct dentry *den = calloc(1, sizeof(*den));
    snprintf(den->name, sizeof(den->name), "%s", "i2c1");
    den->type = DENTRY_TYPE_FILE;
    den->inode = inode;
    den->parent = NULL;
    INIT_LIST_HEAD(&den->childs);
    INIT_LIST_HEAD(&den->child_node);
    dentry_add("/dev", den);

    // i2c_dev1
    i2c_dev1.inode = inode;
    i2c_dev1.dentry = den;
    i2c_dev1.i2c = I2C1;
}

void i2c_init(void)
{
    I2C1_init();
}
