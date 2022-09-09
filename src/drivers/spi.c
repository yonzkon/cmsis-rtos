#include "stm32f1xx_ll_bus.h"
#include "stm32f1xx_ll_gpio.h"
#include "stm32f1xx_ll_spi.h"
#include <stdlib.h>
#include <stdio.h>
#include <printk.h>
#include <fs/fs.h>

struct spi_struct {
    struct inode *inode;
};

static struct spi_struct spi1;

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
    return 0;
}

static int spi_read(struct inode *inode, void *buf, uint32_t size)
{
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
    LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_GPIOB);

    /*
     * SPI1 GPIO Configuration
     * PA4     ------> SPI1_NSS
     * PA5     ------> SPI1_SCK
     * PA6     ------> SPI1_MISO
     * PA7     ------> SPI1_MOSI
     * PB1     ------> SPI1_RST
     */

    LL_GPIO_InitTypeDef GPIO_InitStruct = {0};

    GPIO_InitStruct.Pin = LL_GPIO_PIN_4;
    GPIO_InitStruct.Mode = LL_GPIO_MODE_OUTPUT;
    GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
    LL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = LL_GPIO_PIN_5|LL_GPIO_PIN_6|LL_GPIO_PIN_7;
    GPIO_InitStruct.Mode = LL_GPIO_MODE_ALTERNATE;
    GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
    LL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = LL_GPIO_PIN_1;
    GPIO_InitStruct.Mode = LL_GPIO_MODE_OUTPUT;
    GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
    LL_GPIO_Init(GPIOB, &GPIO_InitStruct);

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

void spi_init(void)
{
    SPI1_init();
}

uint8_t SPI1_read_send_byte(uint8_t byte)
{
    while((SPI1->SR & SPI_SR_TXE) == RESET);
    SPI1->DR = byte;
    while((SPI1->SR & SPI_SR_RXNE) == RESET);
    return SPI1->DR;
}
