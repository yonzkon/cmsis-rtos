#include "stm32f1xx_ll_bus.h"
#include "stm32f1xx_ll_gpio.h"
#include "stm32f1xx_ll_usart.h"
#include <assert.h>
#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <printk.h>
#include <ringbuf.h>
#include <fs/fs.h>

struct uart_device {
    struct inode *inode;
    struct dentry *dentry;
    struct ringbuf *rxbuf;
    USART_TypeDef *usart;
    uint8_t irq;
} uart_dev1, uart_dev2;

void __uart_write_byte(struct uart_device *device, uint8_t byte)
{
    while (!LL_USART_IsActiveFlag_TXE(device->usart));
    LL_USART_TransmitData8(device->usart, byte);
}

int __uart_write(struct uart_device *device, const void *buf, size_t len)
{
    for (int i = 0; i < len; i++)
        __uart_write_byte(device, ((uint8_t *)buf)[i]);
    return len;
}

int __uart_read(struct uart_device *device, void *buf, size_t len)
{
    NVIC_DisableIRQ(device->irq);
    int n = ringbuf_used(device->rxbuf);
    if (n > len) n = len;
    ringbuf_read(device->rxbuf, buf, len);
    NVIC_EnableIRQ(device->irq);
    return n;
}

void USART1_IRQHandler(void)
{
    if (LL_USART_IsActiveFlag_RXNE(USART1))
        ringbuf_write_byte(uart_dev1.rxbuf, USART1->DR & USART_DR_DR);
}

void USART2_IRQHandler(void)
{
    if (LL_USART_IsActiveFlag_RXNE(USART2))
        ringbuf_write_byte(uart_dev2.rxbuf, USART2->DR & USART_DR_DR);
}

static int uart_open(struct file *filp)
{
    if (strcmp(filp->dentry->name, "ttyS1") == 0) {
        filp->private_data = &uart_dev1;
    } else if (strcmp(filp->dentry->name, "ttyS2") == 0) {
        filp->private_data = &uart_dev2;
    } else {
        assert(0);
    }

    return 0;
}

static int uart_close(struct file *filp)
{
    return 0;
}

static int uart_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
    return 0;
}

static int uart_write(struct file *filp, const void *buf, uint32_t len)
{
    struct uart_device *device = filp->private_data;
    return __uart_write(device, buf, len);
}

static int uart_read(struct file *filp, void *buf, uint32_t len)
{
    struct uart_device *device = filp->private_data;
    return __uart_read(device, buf, len);
}

static struct file_operations uart_fops =  {
    .open = uart_open,
    .close = uart_close,
    .ioctl = uart_ioctl,
    .write = uart_write,
    .read = uart_read,
};

static void USART1_init(void)
{
    LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_USART1);
    LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_GPIOA);

    /*
     * USART1 GPIO Configuration
     * PA9     ------> USART1_TX
     * PA10    ------> USART1_RX
     */

    LL_GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = LL_GPIO_PIN_9;
    GPIO_InitStruct.Mode = LL_GPIO_MODE_ALTERNATE;
    GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
    LL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = LL_GPIO_PIN_10;
    GPIO_InitStruct.Mode = LL_GPIO_MODE_FLOATING;
    LL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    NVIC_SetPriority(USART1_IRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(),0, 0));
    NVIC_EnableIRQ(USART1_IRQn);

    LL_USART_InitTypeDef USART_InitStruct = {0};
    USART_InitStruct.BaudRate = 115200;
    USART_InitStruct.DataWidth = LL_USART_DATAWIDTH_8B;
    USART_InitStruct.StopBits = LL_USART_STOPBITS_1;
    USART_InitStruct.Parity = LL_USART_PARITY_NONE;
    USART_InitStruct.TransferDirection = LL_USART_DIRECTION_TX_RX;
    USART_InitStruct.HardwareFlowControl = LL_USART_HWCONTROL_NONE;
    USART_InitStruct.OverSampling = LL_USART_OVERSAMPLING_16;
    LL_USART_Init(USART1, &USART_InitStruct);
    LL_USART_ConfigAsyncMode(USART1);
    LL_USART_Enable(USART1);

    // fs init
    struct inode *inode = calloc(1, sizeof(*inode));
    inode->type = INODE_TYPE_CHAR;
    inode->f_ops = &uart_fops;
    INIT_LIST_HEAD(&inode->node);
    struct dentry *den = calloc(1, sizeof(*den));
    snprintf(den->name, sizeof(den->name), "%s", "ttyS1");
    den->type = DENTRY_TYPE_FILE;
    den->inode = inode;
    den->parent = NULL;
    INIT_LIST_HEAD(&den->childs);
    INIT_LIST_HEAD(&den->child_node);
    dentry_add("/dev", den);

    // uart_dev1
    uart_dev1.inode = inode;
    uart_dev1.dentry = den;
    uart_dev1.rxbuf = ringbuf_new(1024);
    uart_dev1.usart = USART1;
    uart_dev1.irq = USART1_IRQn;
    LL_USART_EnableIT_RXNE(USART1);
}

static void USART2_init(void)
{
    LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_USART2);
    LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_GPIOA);

    /*
     * USART2 GPIO Configuration
     * PA2     ------> USART2_TX
     * PA3     ------> USART2_RX
     */

    LL_GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = LL_GPIO_PIN_2;
    GPIO_InitStruct.Mode = LL_GPIO_MODE_ALTERNATE;
    GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
    LL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = LL_GPIO_PIN_3;
    GPIO_InitStruct.Mode = LL_GPIO_MODE_FLOATING;
    LL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    NVIC_SetPriority(USART2_IRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(),0, 0));
    NVIC_EnableIRQ(USART2_IRQn);

    LL_USART_InitTypeDef USART_InitStruct = {0};
    USART_InitStruct.BaudRate = 115200;
    USART_InitStruct.DataWidth = LL_USART_DATAWIDTH_8B;
    USART_InitStruct.StopBits = LL_USART_STOPBITS_1;
    USART_InitStruct.Parity = LL_USART_PARITY_NONE;
    USART_InitStruct.TransferDirection = LL_USART_DIRECTION_TX_RX;
    USART_InitStruct.HardwareFlowControl = LL_USART_HWCONTROL_NONE;
    USART_InitStruct.OverSampling = LL_USART_OVERSAMPLING_16;
    LL_USART_Init(USART2, &USART_InitStruct);
    LL_USART_ConfigAsyncMode(USART2);
    LL_USART_Enable(USART2);

    // fs init
    struct inode *inode = calloc(1, sizeof(*inode));
    inode->type = INODE_TYPE_CHAR;
    inode->f_ops = &uart_fops;
    INIT_LIST_HEAD(&inode->node);
    struct dentry *den = calloc(1, sizeof(*den));
    snprintf(den->name, sizeof(den->name), "%s", "ttyS2");
    den->type = DENTRY_TYPE_FILE;
    den->inode = inode;
    den->parent = NULL;
    INIT_LIST_HEAD(&den->childs);
    INIT_LIST_HEAD(&den->child_node);
    dentry_add("/dev", den);

    // uart_dev2
    uart_dev2.inode = inode;
    uart_dev2.dentry = den;
    uart_dev2.rxbuf = ringbuf_new(1024);
    uart_dev2.usart = USART2;
    uart_dev2.irq = USART2_IRQn;
    LL_USART_EnableIT_RXNE(USART2);
}

void uart_init(void)
{
    USART1_init();
    USART2_init();
}
