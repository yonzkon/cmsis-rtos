#include "stm32f1xx_ll_bus.h"
#include "stm32f1xx_ll_gpio.h"
#include "stm32f1xx_ll_i2c.h"
#include <stdlib.h>
#include <stdio.h>
#include <printk.h>
#include <fs/fs.h>

struct i2c_struct {
    struct inode *inode;
};

static struct i2c_struct i2c1;

static int i2c_open(struct inode *inode)
{
    return 0;
}

static int i2c_close(struct inode *inode)
{
    return 0;
}

static int i2c_ioctl(struct inode *inode, unsigned int cmd, unsigned long arg)
{
    return 0;
}

static int i2c_write(struct inode *inode, const void *buf, uint32_t len)
{
    return 0;
}

static int i2c_read(struct inode *inode, void *buf, uint32_t len)
{
    return 0;
}

static inode_ops_t i2c_ops =  {
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
    i2c1.inode = calloc(1, sizeof(*i2c1.inode));
    i2c1.inode->type = INODE_TYPE_CHAR;
    i2c1.inode->ops = i2c_ops;
    INIT_LIST_HEAD(&i2c1.inode->node);
    struct dentry *den1 = calloc(1, sizeof(*den1));
    snprintf(den1->name, sizeof(den1->name), "%s", "i2c1");
    den1->type = DENTRY_TYPE_FILE;
    den1->parent = NULL;
    INIT_LIST_HEAD(&den1->childs);
    INIT_LIST_HEAD(&den1->child_node);
    den1->inode = i2c1.inode;
    dentry_add("/dev", den1);
}

void i2c_init(void)
{
    I2C1_init();
}

void I2C1_write_byte(uint8_t dev, uint8_t addr, uint8_t byte)
{
    while (LL_I2C_IsActiveFlag_BUSY(I2C1));

    //I2C1->CR1 |= I2C_CR1_START;
    LL_I2C_GenerateStartCondition(I2C1);
    while (!LL_I2C_IsActiveFlag_SB(I2C1));

    LL_I2C_TransmitData8(I2C1, dev << 1);
    while (!LL_I2C_IsActiveFlag_ADDR(I2C1));
    LL_I2C_ClearFlag_ADDR(I2C1);

    //I2C1->DR = 0x10;
    LL_I2C_TransmitData8(I2C1, addr);
    while (!LL_I2C_IsActiveFlag_TXE(I2C1));

    //I2C1->DR = 0xcc;
    LL_I2C_TransmitData8(I2C1, byte);
    while (!LL_I2C_IsActiveFlag_BTF(I2C1));

    I2C1->CR1 |= I2C_CR1_STOP;
}

uint8_t I2C1_read_byte(uint8_t dev, uint8_t addr)
{
    while(LL_I2C_IsActiveFlag_BUSY(I2C1));

    LL_I2C_AcknowledgeNextData(I2C1, LL_I2C_ACK);
    LL_I2C_GenerateStartCondition(I2C1);
    while(!LL_I2C_IsActiveFlag_SB(I2C1));

    LL_I2C_TransmitData8(I2C1, dev);
    while(!LL_I2C_IsActiveFlag_ADDR(I2C1));
    LL_I2C_ClearFlag_ADDR(I2C1);

    while(!LL_I2C_IsActiveFlag_TXE(I2C1));
    LL_I2C_TransmitData8(I2C1, addr);
    while(!LL_I2C_IsActiveFlag_TXE(I2C1));

    LL_I2C_GenerateStopCondition(I2C1);
    LL_I2C_GenerateStartCondition(I2C1);
    while(!LL_I2C_IsActiveFlag_SB(I2C1));

    LL_I2C_TransmitData8(I2C1, dev | 0x01);
    while(!LL_I2C_IsActiveFlag_ADDR(I2C1));

    LL_I2C_AcknowledgeNextData(I2C1, LL_I2C_NACK);
    LL_I2C_ClearFlag_ADDR(I2C1);
    LL_I2C_GenerateStopCondition(I2C1);

    while(!LL_I2C_IsActiveFlag_RXNE(I2C1));
    return LL_I2C_ReceiveData8(I2C1);
}
