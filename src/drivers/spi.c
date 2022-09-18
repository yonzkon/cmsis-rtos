#include "stm32f1xx_ll_bus.h"
#include "stm32f1xx_ll_gpio.h"
#include "stm32f1xx_ll_spi.h"
#include <assert.h>
#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <printk.h>
#include <fs/fs.h>

void SPI_cs_sel(SPI_TypeDef *SPIx)
{
    if (SPIx == SPI1)
        LL_GPIO_ResetOutputPin(GPIOA, LL_GPIO_PIN_4);
    else if (SPIx == SPI2)
        LL_GPIO_ResetOutputPin(GPIOB, LL_GPIO_PIN_12);
}

void SPI_cs_desel(SPI_TypeDef *SPIx)
{
    if (SPIx == SPI1)
        LL_GPIO_SetOutputPin(GPIOA, LL_GPIO_PIN_4);
    else if (SPIx == SPI2)
        LL_GPIO_SetOutputPin(GPIOB, LL_GPIO_PIN_12);
}

static uint8_t SPI_read_write_byte(SPI_TypeDef *SPIx, uint8_t byte)
{
    while((SPIx->SR & SPI_SR_TXE) == RESET);
    SPIx->DR = byte;
    while((SPIx->SR & SPI_SR_RXNE) == RESET);
    return SPIx->DR;
}

uint8_t SPI_read_byte(SPI_TypeDef *SPIx)
{
    return SPI_read_write_byte(SPIx, 0x00);
}

void SPI_write_byte(SPI_TypeDef *SPIx, uint8_t byte)
{
    SPI_read_write_byte(SPIx, byte);
}

int SPI_read(SPI_TypeDef *SPIx, void *buf, uint32_t len)
{
    for (int i = 0; i < len; i++)
        ((uint8_t *)buf)[i] = SPI_read_write_byte(SPIx, 0x00);
    return len;
}

int SPI_write(SPI_TypeDef *SPIx, const void *buf, uint32_t len)
{
    for (int i = 0; i < len; i++)
        SPI_read_write_byte(SPIx, ((uint8_t *)buf)[i]);
    return len;
}

static struct spi_device {
    struct inode *inode;
    struct dentry *dentry;
    SPI_TypeDef *spi;
} spi_dev1, spi_dev2;

static int spi_open(struct file *filp)
{
    if (strcmp(filp->dentry->name, "spi1") == 0) {
        filp->private_data = &spi_dev1;
    } else if (strcmp(filp->dentry->name, "spi2") == 0) {
        filp->private_data = &spi_dev2;
    } else {
        assert(0);
    }

    return 0;
}

static int spi_close(struct file *filp)
{
    return 0;
}

static int spi_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
    return 0;
}

static int spi_write(struct file *filp, const void *buf, uint32_t len)
{
    struct spi_device *device = filp->private_data;

    SPI_cs_sel(device->spi);
    int rc = SPI_write(device->spi, buf, len);
    SPI_cs_desel(device->spi);
    return rc;
}

static int spi_read(struct file *filp, void *buf, uint32_t len)
{
    struct spi_device *device = filp->private_data;

    SPI_cs_sel(device->spi);
    int rc = SPI_read(device->spi, buf, len);
    SPI_cs_desel(device->spi);
    return rc;
}

static struct file_operations spi_fops =  {
    .open = spi_open,
    .close = spi_close,
    .ioctl = spi_ioctl,
    .write = spi_write,
    .read = spi_read,
};

static void SPI1_init(void)
{
    LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_SPI1);
    LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_GPIOA);

    /*
     * SPI1 GPIO Configuration
     * PA4     ------> SPI1_NSS
     * PA5     ------> SPI1_SCK
     * PA6     ------> SPI1_MISO
     * PA7     ------> SPI1_MOSI
     */

    LL_GPIO_InitTypeDef GPIO_InitStruct = {0};

    // NSS
    GPIO_InitStruct.Pin = LL_GPIO_PIN_4;
    GPIO_InitStruct.Mode = LL_GPIO_MODE_OUTPUT;
    GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
    LL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    // SCK, MISO, MOSI
    GPIO_InitStruct.Pin = LL_GPIO_PIN_5|LL_GPIO_PIN_6|LL_GPIO_PIN_7;
    GPIO_InitStruct.Mode = LL_GPIO_MODE_ALTERNATE;
    GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
    LL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    LL_SPI_InitTypeDef SPI_InitStruct = {0};
    SPI_InitStruct.TransferDirection = LL_SPI_FULL_DUPLEX;
    SPI_InitStruct.Mode = LL_SPI_MODE_MASTER;
    SPI_InitStruct.DataWidth = LL_SPI_DATAWIDTH_8BIT;
    SPI_InitStruct.ClockPolarity = LL_SPI_POLARITY_HIGH;
    SPI_InitStruct.ClockPhase = LL_SPI_PHASE_2EDGE;
    SPI_InitStruct.NSS = LL_SPI_NSS_SOFT;
    SPI_InitStruct.BaudRate = LL_SPI_BAUDRATEPRESCALER_DIV4;
    SPI_InitStruct.BitOrder = LL_SPI_MSB_FIRST;
    SPI_InitStruct.CRCCalculation = LL_SPI_CRCCALCULATION_DISABLE;
    SPI_InitStruct.CRCPoly = 7;
    LL_SPI_Init(SPI1, &SPI_InitStruct);
    LL_SPI_Enable(SPI1);

    // fs init
    struct inode *inode = calloc(1, sizeof(*inode));
    inode->type = INODE_TYPE_CHAR;
    inode->f_ops = &spi_fops;
    struct dentry *den = calloc(1, sizeof(*den));
    snprintf(den->name, sizeof(den->name), "%s", "spi1");
    den->type = DENTRY_TYPE_FILE;
    den->inode = inode;
    den->parent = NULL;
    INIT_LIST_HEAD(&den->childs);
    INIT_LIST_HEAD(&den->child_node);
    dentry_add("/dev", den);

    // spi_device
    spi_dev1.inode = inode;
    spi_dev1.dentry = den;
    spi_dev1.spi = SPI1;
}

static void SPI2_init(void)
{
    LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_SPI2);
    LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_GPIOB);

    /*
     * SPI2 GPIO Configuration
     * PB12    ------> SPI2_NSS
     * PB13    ------> SPI2_SCK
     * PB14    ------> SPI2_MISO
     * PB15    ------> SPI2_MOSI
     */

    LL_GPIO_InitTypeDef GPIO_InitStruct = {0};

    // NSS
    GPIO_InitStruct.Pin = LL_GPIO_PIN_12;
    GPIO_InitStruct.Mode = LL_GPIO_MODE_OUTPUT;
    GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
    LL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    // SCK, MOSI
    GPIO_InitStruct.Pin = LL_GPIO_PIN_13|LL_GPIO_PIN_14|LL_GPIO_PIN_15;
    GPIO_InitStruct.Mode = LL_GPIO_MODE_ALTERNATE;
    GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
    LL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    LL_SPI_InitTypeDef SPI_InitStruct = {0};
    SPI_InitStruct.TransferDirection = LL_SPI_FULL_DUPLEX;
    SPI_InitStruct.Mode = LL_SPI_MODE_MASTER;
    SPI_InitStruct.DataWidth = LL_SPI_DATAWIDTH_8BIT;
    SPI_InitStruct.ClockPolarity = LL_SPI_POLARITY_LOW;
    SPI_InitStruct.ClockPhase = LL_SPI_PHASE_1EDGE;
    SPI_InitStruct.NSS = LL_SPI_NSS_SOFT;
    SPI_InitStruct.BaudRate = LL_SPI_BAUDRATEPRESCALER_DIV128;
    SPI_InitStruct.BitOrder = LL_SPI_MSB_FIRST;
    SPI_InitStruct.CRCCalculation = LL_SPI_CRCCALCULATION_DISABLE;
    SPI_InitStruct.CRCPoly = 7;
    LL_SPI_Init(SPI2, &SPI_InitStruct);
    LL_SPI_Enable(SPI2);

    // fs init
    struct inode *inode = calloc(1, sizeof(*inode));
    inode->type = INODE_TYPE_CHAR;
    inode->f_ops = &spi_fops;
    struct dentry *den = calloc(1, sizeof(*den));
    snprintf(den->name, sizeof(den->name), "%s", "spi2");
    den->type = DENTRY_TYPE_FILE;
    den->inode = inode;
    den->parent = NULL;
    INIT_LIST_HEAD(&den->childs);
    INIT_LIST_HEAD(&den->child_node);
    dentry_add("/dev", den);

    // spi_device
    spi_dev2.inode = inode;
    spi_dev2.dentry = den;
    spi_dev2.spi = SPI2;
}

void spi_init(void)
{
    SPI1_init();
    SPI2_init();
}
