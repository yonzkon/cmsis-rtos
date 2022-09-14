#include "stm32f1xx_ll_bus.h"
#include "stm32f1xx_ll_gpio.h"
#include "stm32f1xx_ll_spi.h"
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <printk.h>
#include <fs/fs.h>

void SPI1_cs_sel(void)
{
    LL_GPIO_ResetOutputPin(GPIOA, LL_GPIO_PIN_4);
}

void SPI1_cs_desel(void)
{
    LL_GPIO_SetOutputPin(GPIOA, LL_GPIO_PIN_4);
}

static uint8_t SPI1_read_write_byte(uint8_t byte)
{
    while((SPI1->SR & SPI_SR_TXE) == RESET);
    SPI1->DR = byte;
    while((SPI1->SR & SPI_SR_RXNE) == RESET);
    return SPI1->DR;
}

uint8_t SPI1_read_byte()
{
    return SPI1_read_write_byte(0x00);
}

void SPI1_write_byte(uint8_t byte)
{
    SPI1_read_write_byte(byte);
}

int SPI1_read(void *buf, uint32_t len)
{
    for (int i = 0; i < len; i++)
        ((uint8_t *)buf)[len] = SPI1_read_write_byte(0x00);
    return len;
}

int SPI1_write(const void *buf, uint32_t len)
{
    for (int i = 0; i < len; i++)
        SPI1_read_write_byte(((uint8_t *)buf)[len]);
    return len;
}

void SPI2_cs_sel(void)
{
    LL_GPIO_ResetOutputPin(GPIOA, LL_GPIO_PIN_4);
}

void SPI2_cs_desel(void)
{
    LL_GPIO_SetOutputPin(GPIOA, LL_GPIO_PIN_4);
}

static uint8_t SPI2_read_write_byte(uint8_t byte)
{
    while((SPI2->SR & SPI_SR_TXE) == RESET);
    SPI2->DR = byte;
    while((SPI2->SR & SPI_SR_RXNE) == RESET);
    return SPI2->DR;
}

uint8_t SPI2_read_byte()
{
    return SPI2_read_write_byte(0x00);
}

void SPI2_write_byte(uint8_t byte)
{
    SPI2_read_write_byte(byte);
}

int SPI2_read(void *buf, uint32_t len)
{
    for (int i = 0; i < len; i++)
        ((uint8_t *)buf)[len] = SPI2_read_write_byte(0x00);
    return len;
}

int SPI2_write(const void *buf, uint32_t len)
{
    for (int i = 0; i < len; i++)
        SPI2_read_write_byte(((uint8_t *)buf)[len]);
    return len;
}

static struct spi_struct {
    struct inode *inode;
} spi1, spi2;

static int spi_open(struct inode *inode)
{
    return 0;
}

static int spi_close(struct inode *inode)
{
    return 0;
}

static int spi_ioctl(struct inode *inode, unsigned int cmd, unsigned long arg)
{
    return 0;
}

static int spi_write(struct inode *inode, const void *buf, uint32_t len)
{
    if (inode == spi1.inode) {
        SPI1_cs_sel();
        SPI1_write(buf, len);
        SPI1_cs_desel();
    } else if (inode == spi2.inode) {
        SPI2_cs_sel();
        SPI2_write(buf, len);
        SPI2_cs_desel();
    }
    return 0;
}

static int spi_read(struct inode *inode, void *buf, uint32_t len)
{
    if (inode == spi1.inode) {
        return SPI1_read(buf, len);
    } else if (inode == spi2.inode) {
        return SPI2_read(buf, len);
    }
    return 0;
}

static inode_ops_t spi_ops =  {
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
    spi1.inode = calloc(1, sizeof(*spi1.inode));
    spi1.inode->type = INODE_TYPE_CHAR;
    spi1.inode->ops = spi_ops;
    INIT_LIST_HEAD(&spi1.inode->node);
    struct dentry *den1 = calloc(1, sizeof(*den1));
    snprintf(den1->name, sizeof(den1->name), "%s", "spi1");
    den1->type = DENTRY_TYPE_FILE;
    den1->parent = NULL;
    INIT_LIST_HEAD(&den1->childs);
    INIT_LIST_HEAD(&den1->child_node);
    den1->inode = spi1.inode;
    dentry_add("/dev", den1);
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
    spi2.inode = calloc(1, sizeof(*spi2.inode));
    spi2.inode->type = INODE_TYPE_CHAR;
    spi2.inode->ops = spi_ops;
    INIT_LIST_HEAD(&spi2.inode->node);
    struct dentry *den2 = calloc(1, sizeof(*den2));
    snprintf(den2->name, sizeof(den2->name), "%s", "spi2");
    den2->type = DENTRY_TYPE_FILE;
    den2->parent = NULL;
    INIT_LIST_HEAD(&den2->childs);
    INIT_LIST_HEAD(&den2->child_node);
    den2->inode = spi2.inode;
    dentry_add("/dev", den2);
}

void spi_init(void)
{
    SPI1_init();
    SPI2_init();
}
