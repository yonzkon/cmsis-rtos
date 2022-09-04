#include "stm32f1xx_hal.h"
#include <stdlib.h>
#include <stdio.h>
#include <printk.h>
#include <fs/fs.h>

static SPI_HandleTypeDef hspi1;

void HAL_SPI_MspInit(SPI_HandleTypeDef* hspi)
{
    if (hspi->Instance == SPI1) {
        GPIO_InitTypeDef GPIO_InitStruct = {0};

        __HAL_RCC_SPI1_CLK_ENABLE();
        __HAL_RCC_GPIOA_CLK_ENABLE();

        /*
         * SPI1 GPIO Configuration
         * PA5     ------> SPI1_SCK
         * PA6     ------> SPI1_MISO
         * PA7     ------> SPI1_MOSI
         */

        GPIO_InitStruct.Pin = GPIO_PIN_5|GPIO_PIN_7;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
        HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

        GPIO_InitStruct.Pin = GPIO_PIN_6;
        GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    }
}

void HAL_SPI_MspDeInit(SPI_HandleTypeDef* hspi)
{
    if (hspi->Instance == SPI1) {
        __HAL_RCC_SPI1_CLK_DISABLE();

        /*
         * SPI1 GPIO Configuration
         * PA5     ------> SPI1_SCK
         * PA6     ------> SPI1_MISO
         * PA7     ------> SPI1_MOSI
         */

        HAL_GPIO_DeInit(GPIOA, GPIO_PIN_5|GPIO_PIN_6|GPIO_PIN_7);
    }
}

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

void spi_init(void)
{
    hspi1.Instance = SPI1;
    hspi1.Init.Mode = SPI_MODE_MASTER;
    hspi1.Init.Direction = SPI_DIRECTION_2LINES;
    hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
    hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
    hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
    hspi1.Init.NSS = SPI_NSS_SOFT;
    hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_2;
    hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
    hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
    hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
    hspi1.Init.CRCPolynomial = 10;
    if (HAL_SPI_Init(&hspi1) != HAL_OK)
        panic("init hspi1 failed");

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
